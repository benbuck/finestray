// Copyright 2020 Benbuck Nason
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// App
#include "AboutDialog.h"
#include "AppName.h"
#include "Bitmap.h"
#include "BitmapHandleWrapper.h"
#include "COMLibraryWrapper.h"
#include "ContextMenu.h"
#include "File.h"
#include "HandleWrapper.h"
#include "Helpers.h"
#include "Hotkey.h"
#include "IconHandleWrapper.h"
#include "Log.h"
#include "MenuHandleWrapper.h"
#include "Modifiers.h"
#include "Path.h"
#include "Resource.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "StringUtility.h"
#include "TrayIcon.h"
#include "WinEventHookHandleWrapper.h"
#include "WindowHandleWrapper.h"
#include "WindowIcon.h"
#include "WindowInfo.h"
#include "WindowMessage.h"
#include "WindowTracker.h"

// Windows
#include <CommCtrl.h>
#include <Psapi.h>
#include <WinUser.h>
#include <Windows.h>

// Standard library
#include <cassert>
#include <ranges>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace
{

enum class HotkeyID : unsigned int
{
    Minimize = 1,
    MinimizeAll,
    Restore,
    RestoreAll,
    Menu
};

LRESULT wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
ErrorContext start();
void stop() noexcept;
bool windowShouldAutoTray(HWND hwnd, TrayEvent trayEvent, MinimizePersistence * minimizePersistence);
void minimizeAllWindows();
void minimizeWindow(HWND hwnd, MinimizePersistence minimizePersistence);
void restoreAllWindows();
void restoreWindow(HWND hwnd);
void restoreLastWindow();
void onAddWindow(HWND hwnd);
void onMinimizeEvent(
    HWINEVENTHOOK hwineventhook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime);
bool readSettingsFromFile(const std::string & fileName, Settings & settings);
bool writeSettingsToFile(const std::string & fileName, const Settings & settings);
void showSettingsDialog();
void toggleSettingsDialog();
void onSettingsDialogComplete(bool success, const Settings & settings);
std::string getSettingsFileName();
std::string getStartupShortcutFullPath();
void updateStartWithWindowsShortcut();

WindowHandleWrapper appWindow_;
TrayIcon trayIcon_;
WindowHandleWrapper settingsDialogWindow_;
bool contextMenuActive_;
Settings settings_;
Hotkey hotkeyMinimize_;
Hotkey hotkeyMinimizeAll_;
Hotkey hotkeyRestore_;
Hotkey hotkeyRestoreAll_;
Hotkey hotkeyMenu_;
UINT modifiersOverride_;
UINT taskbarCreatedMessage_;
UINT shellHookMsg_;

} // anonymous namespace

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 26461)
#endif
int WINAPI wWinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nShowCmd)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
{
    // unused
    (void)hPrevInstance;
    (void)pCmdLine;

#if defined(_DEBUG)
    // enable memory leak checking
    // NOLINTBEGIN
    int crtDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    crtDbgFlag |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
    crtDbgFlag &= ~_CRTDBG_CHECK_CRT_DF;
    _CrtSetDbgFlag(crtDbgFlag);
    // NOLINTEND
#endif

    // check if already running
    HWND oldHwnd = FindWindowA(APP_NAME, nullptr);
    if (oldHwnd) {
        INFO_PRINTF("already running\n");
        SendMessageA(oldHwnd, WM_SHOWSETTINGS, 0, 0);
        return 0;
    }

    DEBUG_PRINTF("initializing COM\n");
    const COMLibraryWrapper comLibrary;
    if (!comLibrary.initialized()) {
        errorMessage(IDS_ERROR_INIT_COM);
        return IDS_ERROR_INIT_COM;
    }

    DEBUG_PRINTF("initializing common controls\n");
    INITCOMMONCONTROLSEX initCommonControlsEx;
    initCommonControlsEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    initCommonControlsEx.dwICC = ICC_LISTVIEW_CLASSES;
    if (!InitCommonControlsEx(&initCommonControlsEx)) {
        errorMessage(IDS_ERROR_INIT_COMMON_CONTROLS);
        return IDS_ERROR_INIT_COMMON_CONTROLS;
    }

    // get settings from file
    settings_.initDefaults();
    const std::string settingsFile = getSettingsFileName();
    if (readSettingsFromFile(settingsFile, settings_)) {
        DEBUG_PRINTF("read settings from %s\n", settingsFile.c_str());
        updateStartWithWindowsShortcut();
    } else {
        if (Settings::fileExists(settingsFile)) {
            errorMessage(ErrorContext(IDS_ERROR_LOAD_SETTINGS, settingsFile));
            return IDS_ERROR_LOAD_SETTINGS;
        }

        // no settings file, update start with windows setting to match reality
        const std::string startupShortcutFullPath = getStartupShortcutFullPath();
        const bool startupShortcutExists = fileExists(startupShortcutFullPath);
        if (settings_.startWithWindows_ != startupShortcutExists) {
            INFO_PRINTF("updating start with windows setting to %s\n", StringUtility::boolToCString(startupShortcutExists));
            settings_.startWithWindows_ = startupShortcutExists;
        }
    }

    settings_.dump();

    IconHandleWrapper icon(LoadIcon(hinstance, MAKEINTRESOURCE(IDI_FINESTRAY)), IconHandleWrapper::Mode::Referenced);

    DEBUG_PRINTF("registering window class\n");
    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hinstance;
    wc.lpszClassName = APP_NAME;
    wc.hIcon = icon;
    wc.hIconSm = icon;
    ATOM const atom = RegisterClassExA(&wc);
    if (!atom) {
        const std::string lastErrorString = StringUtility::lastErrorString();
        ERROR_PRINTF("could not create window class, RegisterClassExA() failed: %s", lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_REGISTER_WINDOW_CLASS, lastErrorString));
        return IDS_ERROR_REGISTER_WINDOW_CLASS;
    }

    DEBUG_PRINTF("creating window\n");
    appWindow_ = CreateWindowA(
        APP_NAME, // class name
        APP_NAME, // window name
        WS_OVERLAPPEDWINDOW, // style
        0, // x
        0, // y
        0, // w
        0, // h
        nullptr, // parent
        nullptr, // menu
        hinstance, // instance
        nullptr // application data
    );
    if (!appWindow_) {
        const std::string lastErrorString = StringUtility::lastErrorString();
        ERROR_PRINTF("could not create window, CreateWindowA() failed: %s", lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_CREATE_WINDOW, lastErrorString));
        return IDS_ERROR_CREATE_WINDOW;
    }

    // we intentionally don't show the window
    // ShowWindow(appWindow_, nShowCmd);
    (void)nShowCmd;

    ErrorContext err;

    DEBUG_PRINTF("creating tray icon for app\n");
    err = trayIcon_.create(appWindow_, appWindow_, WM_TRAYWINDOW, std::move(icon));
    if (err) {
        // this error can happen legitimately if the taskbar hasn't been created
        // log an error, and assume we will create the icon when we get the TaskbarCreated message
        ERROR_PRINTF("failed to create tray icon, TrayIcon::create() failed: %s\n", err.errorString().c_str());
    }

    DEBUG_PRINTF("registering event hook to monitor minimize events\n");
    WinEventHookHandleWrapper minimizeEventHook(SetWinEventHook(
        EVENT_SYSTEM_MINIMIZESTART,
        EVENT_SYSTEM_MINIMIZESTART,
        nullptr,
        onMinimizeEvent,
        0,
        0,
        WINEVENT_OUTOFCONTEXT));
    if (!minimizeEventHook) {
        const std::string lastErrorString = StringUtility::lastErrorString();
        ERROR_PRINTF(
            "failed to hook minimize win event %#x, SetWinEventHook() failed: %s\n",
            appWindow_.hwnd(),
            lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_REGISTER_EVENTHOOK, lastErrorString));
        return IDS_ERROR_REGISTER_EVENTHOOK;
    }

    WindowTracker::start(appWindow_);

    DEBUG_PRINTF("starting\n");
    err = start();
    if (err) {
        errorMessage(err);
        INFO_PRINTF("start error, showing settings dialog\n");
        showSettingsDialog();
    } else {
        if (!Settings::fileExists(settingsFile)) {
            INFO_PRINTF("no settings file, showing settings dialog\n");
            showSettingsDialog();
        }
    }

    RegisterShellHookWindow(appWindow_);
    shellHookMsg_ = RegisterWindowMessage(L"SHELLHOOK");

    DEBUG_PRINTF("running message loop\n");
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        // needed to have working tab stops in the settings dialog
        if (settingsDialogWindow_ && IsDialogMessageA(settingsDialogWindow_, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DEBUG_PRINTF("exiting\n");

    // if there are any minimized windows, restore them
    restoreAllWindows();

    DeregisterShellHookWindow(appWindow_);
    minimizeEventHook.destroy();
    trayIcon_.destroy();
    stop();
    WindowTracker::stop();
    settingsDialogWindow_.destroy();
    appWindow_.destroy();

    return 0;
}

namespace
{

LRESULT wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        // command from context menu
        case WM_COMMAND: {
            WORD const id = LOWORD(wParam);
            switch (id) {
                // about dialog
                case ContextMenu::IDM_APP:
                case ContextMenu::IDM_ABOUT: {
                    INFO_PRINTF("menu about\n");
                    showAboutDialog(hwnd);
                    break;
                }

                case ContextMenu::IDM_MINIMIZE_ALL: {
                    INFO_PRINTF("menu minimize all\n");
                    minimizeAllWindows();
                    break;
                }

                case ContextMenu::IDM_RESTORE_ALL: {
                    INFO_PRINTF("menu restore all\n");
                    restoreAllWindows();
                    break;
                }

                case ContextMenu::IDM_SETTINGS: {
                    INFO_PRINTF("menu settings\n");
                    showSettingsDialog();
                    break;
                }

                // exit the app
                case ContextMenu::IDM_EXIT: {
                    INFO_PRINTF("menu exit\n");
                    PostQuitMessage(0);
                    break;
                }

                default: {
                    HWND minimizedHwnd = ContextMenu::getMinimizedWindow(id);
                    if (minimizedHwnd) {
                        restoreWindow(minimizedHwnd);
                    } else {
                        HWND visibleHwnd = ContextMenu::getVisibleWindow(id);
                        if (visibleHwnd) {
                            minimizeWindow(visibleHwnd, MinimizePersistence::None);
                        }
                    }
                    break;
                }
            }
            break;
        }

        case WM_CREATE: {
            // get the message id to be notified when taskbar is (re-)created
            INFO_PRINTF("registering taskbar created message\n");
            taskbarCreatedMessage_ = RegisterWindowMessage(TEXT("TaskbarCreated"));
            break;
        }

        case WM_DESTROY: {
            INFO_PRINTF("destroying window\n");
            PostQuitMessage(0);
            return 0;
        }

        // our hotkey was activated
        case WM_HOTKEY: {
            const HotkeyID hkid = static_cast<HotkeyID>(wParam);
            switch (hkid) {
                case HotkeyID::Minimize: {
                    INFO_PRINTF("hotkey minimize\n");
                    // get the foreground window to minimize
                    HWND foregroundHwnd = GetForegroundWindow();
                    if (!foregroundHwnd) {
                        WARNING_PRINTF("no foreground window to minimize, ignoring\n");
                    } else {
                        // only minimize windows that have a minimize button
                        LONG const windowStyle = GetWindowLong(foregroundHwnd, GWL_STYLE);
                        if (windowStyle & WS_MINIMIZEBOX) {
                            minimizeWindow(foregroundHwnd, MinimizePersistence::None);
                        }
                    }
                    break;
                }

                case HotkeyID::MinimizeAll: {
                    INFO_PRINTF("hotkey minimize all\n");
                    minimizeAllWindows();
                    break;
                }

                case HotkeyID::Restore: {
                    INFO_PRINTF("hotkey restore\n");
                    restoreLastWindow();
                    break;
                }

                case HotkeyID::RestoreAll: {
                    INFO_PRINTF("hotkey restore all\n");
                    restoreAllWindows();
                    break;
                }

                case HotkeyID::Menu: {
                    INFO_PRINTF("hotkey menu\n");
                    if (contextMenuActive_) {
                        WARNING_PRINTF("context menu already active, ignoring hotkey\n");
                    } else {
                        if (!ContextMenu::show(hwnd, settings_.minimizePlacement_)) {
                            errorMessage(IDS_ERROR_CREATE_MENU);
                        }
                    }
                    break;
                }

                default: {
                    WARNING_PRINTF("invalid hotkey id %d\n", hkid);
                    break;
                }
            }
            break;
        }

        // message from the tray (taskbar) icon
        case WM_TRAYWINDOW: {
            switch (lParam) {
                // user activated context menu
                case WM_CONTEXTMENU: {
                    INFO_PRINTF("tray context menu\n");
                    if (contextMenuActive_) {
                        WARNING_PRINTF("context menu already active, ignoring\n");
                    } else {
                        if (!ContextMenu::show(hwnd, settings_.minimizePlacement_)) {
                            errorMessage(IDS_ERROR_CREATE_MENU);
                        }
                    }
                    break;
                }

                case WM_LBUTTONDOWN: // left click down on icon
                case WM_LBUTTONUP: // left click up on icon
                case WM_MOUSEMOVE: // mouse moved over icon
                case WM_RBUTTONDOWN: // right click down on icon
                case WM_RBUTTONUP: // right click up on icon
                {
                    // nothing to do
                    break;
                }

                // user selected and activated icon
                case NIN_SELECT: {
                    INFO_PRINTF("tray icon selected\n");
                    HWND hwndTray = TrayIcon::getWindowFromID(narrow_cast<UINT>(wParam));
                    if (hwndTray) {
                        if (hwndTray == appWindow_) {
                            INFO_PRINTF("toggling settings dialog\n");
                            toggleSettingsDialog();
                        } else if (WindowTracker::isMinimized(hwndTray)) {
                            INFO_PRINTF("restoring window from tray: %#x\n", hwndTray);
                            restoreWindow(hwndTray);
                        } else {
                            INFO_PRINTF("minimizing window to tray: %#x\n", hwndTray);
                            // must be persistent for this to happen
                            minimizeWindow(hwndTray, MinimizePersistence::None);
                        }
                    } else {
                        WARNING_PRINTF("unknown tray icon id %#x\n", wParam);
                    }
                    break;
                }

                default: {
                    WARNING_PRINTF("unhandled WM_TRAYWINDOW message %x\n", lParam);
                    break;
                }
            }
            break;
        }

        case WM_SHOWSETTINGS: {
            INFO_PRINTF("showing settings dialog\n");
            showSettingsDialog();
            break;
        }

        case WM_ENTERMENULOOP: {
            DEBUG_PRINTF("Context menu active\n");
            contextMenuActive_ = true;
            break;
        }
        case WM_EXITMENULOOP: {
            DEBUG_PRINTF("Context menu inactive\n");
            contextMenuActive_ = false;
            break;
        }

        default: {
            if (uMsg == taskbarCreatedMessage_) {
                INFO_PRINTF("taskbar created\n");
                HINSTANCE hinstance = getInstance();
                IconHandleWrapper icon(
                    LoadIcon(hinstance, MAKEINTRESOURCE(IDI_FINESTRAY)),
                    IconHandleWrapper::Mode::Referenced);
                const ErrorContext err = trayIcon_.create(appWindow_, appWindow_, WM_TRAYWINDOW, std::move(icon));
                if (err) {
                    ERROR_PRINTF("failed to create tray icon, TrayIcon::create() failed: %s\n", err.errorString().c_str());
                }
                WindowTracker::addAllMinimizedToTray(settings_.minimizePlacement_);
            } else if (uMsg == shellHookMsg_) {
                HWND shellHwnd = reinterpret_cast<HWND>(lParam);
                switch (wParam) {
                    case HSHELL_WINDOWCREATED: {
                        INFO_PRINTF("shell hook window created %#x - '%s'\n", shellHwnd, getWindowText(shellHwnd).c_str());
                        onAddWindow(shellHwnd);
                        break;
                    }

                    case HSHELL_WINDOWDESTROYED: {
                        INFO_PRINTF("shell hook destroyed %#x - '%s'\n", shellHwnd, getWindowText(shellHwnd).c_str());
                        if (IsWindow(shellHwnd)) {
                            WindowTracker::windowChanged(shellHwnd);
                        } else {
                            WindowTracker::windowDestroyed(shellHwnd);
                        }
                        break;
                    }

                    case HSHELL_REDRAW: {
                        DEBUG_PRINTF("HSHELL_REDRAW: %#x\n", lParam);
                        WindowTracker::windowChanged(shellHwnd);
                        break;
                    }

                    default: {
                        // DEBUG_PRINTF("unknown shell hook message %x\n", wParam);
                        break;
                    }
                }
            }
            break;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

ErrorContext start()
{
    Log::start(settings_.logToFile_, APP_NAME ".log");

    DEBUG_PRINTF("starting\n");

    // register a hotkey that will be used to minimize windows
    UINT vkMinimize = VK_DOWN;
    UINT modifiersMinimize = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.hotkeyMinimize_, vkMinimize, modifiersMinimize)) {
        return { IDS_ERROR_PARSE_HOTKEY, "minimize" };
    }
    if (!vkMinimize || !modifiersMinimize) {
        INFO_PRINTF("no hotkey to minimize windows\n");
    } else {
        DEBUG_PRINTF("registering hotkey to minimize windows\n");
        if (!hotkeyMinimize_
                 .create(static_cast<INT>(HotkeyID::Minimize), appWindow_, vkMinimize, modifiersMinimize | MOD_NOREPEAT)) {
            return { IDS_ERROR_REGISTER_HOTKEY, "minimize" };
        }
    }

    // register a hotkey that will be used to minimize all windows
    UINT vkMinimizeAll = VK_RIGHT;
    UINT modifiersMinimizeAll = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.hotkeyMinimizeAll_, vkMinimizeAll, modifiersMinimizeAll)) {
        return { IDS_ERROR_PARSE_HOTKEY, "minimize all" };
    }
    if (!vkMinimizeAll || !modifiersMinimizeAll) {
        INFO_PRINTF("no hotkey to minimize all windows\n");
    } else {
        DEBUG_PRINTF("registering hotkey to minimize all windows\n");
        if (!hotkeyMinimizeAll_.create(
                static_cast<INT>(HotkeyID::MinimizeAll),
                appWindow_,
                vkMinimizeAll,
                modifiersMinimizeAll | MOD_NOREPEAT)) {
            return { IDS_ERROR_REGISTER_HOTKEY, "minimize all" };
        }
    }

    // register a hotkey that will be used to restore windows
    UINT vkRestore = VK_UP;
    UINT modifiersRestore = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.hotkeyRestore_, vkRestore, modifiersRestore)) {
        return { IDS_ERROR_PARSE_HOTKEY, "restore" };
    }
    if (!vkRestore || !modifiersRestore) {
        INFO_PRINTF("no hotkey to restore windows\n");
    } else {
        DEBUG_PRINTF("registering hotkey to restore windows\n");
        if (!hotkeyRestore_
                 .create(static_cast<INT>(HotkeyID::Restore), appWindow_, vkRestore, modifiersRestore | MOD_NOREPEAT)) {
            return { IDS_ERROR_REGISTER_HOTKEY, "restore" };
        }
    }

    // register a hotkey that will be used to restore all windows
    UINT vkRestoreAll = VK_LEFT;
    UINT modifiersRestoreAll = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.hotkeyRestoreAll_, vkRestoreAll, modifiersRestoreAll)) {
        return { IDS_ERROR_PARSE_HOTKEY, "restore all" };
    }
    if (!vkRestoreAll || !modifiersRestoreAll) {
        INFO_PRINTF("no hotkey to restore all windows\n");
    } else {
        DEBUG_PRINTF("registering hotkey to restore all windows\n");
        if (!hotkeyRestoreAll_.create(
                static_cast<INT>(HotkeyID::RestoreAll),
                appWindow_,
                vkRestoreAll,
                modifiersRestoreAll | MOD_NOREPEAT)) {
            return { IDS_ERROR_REGISTER_HOTKEY, "restore all" };
        }
    }

    // register a hotkey that will be used to show the context menu
    UINT vkMenu = VK_HOME;
    UINT modifiersMenu = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.hotkeyMenu_, vkMenu, modifiersMenu)) {
        return { IDS_ERROR_PARSE_HOTKEY, "menu" };
    }
    if (!vkMenu || !modifiersMenu) {
        INFO_PRINTF("no hotkey to show context menu\n");
    } else {
        DEBUG_PRINTF("registering hotkey to show context menu\n");
        if (!hotkeyMenu_.create(static_cast<INT>(HotkeyID::Menu), appWindow_, vkMenu, modifiersMenu | MOD_NOREPEAT)) {
            return { IDS_ERROR_REGISTER_HOTKEY, "menu" };
        }
    }

    // get modifiers that will be used to override auto-tray
    UINT vkOverride = 0;
    modifiersOverride_ = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.modifiersOverride_, vkOverride, modifiersOverride_)) {
        return { IDS_ERROR_PARSE_MODIFIER, "override" };
    }
    if (!modifiersOverride_) {
        INFO_PRINTF("no override modifiers\n");
    } else if (
        vkOverride ||
        (modifiersOverride_ &
         ~(static_cast<UINT>(MOD_ALT) | static_cast<UINT>(MOD_CONTROL) | static_cast<UINT>(MOD_SHIFT)))) {
        WARNING_PRINTF("invalid override modifiers\n");
        return { IDS_ERROR_REGISTER_MODIFIER, "override" };
    }

    // check auto-tray regular expressions to surface an error if needed
    for (const Settings::AutoTray & autoTray : settings_.autoTrays_) {
        try {
            const std::regex re(autoTray.windowTitle_);
            static_cast<void>(re);
        } catch (const std::regex_error & e) {
            return { IDS_ERROR_PARSE_REGEX, "'" + autoTray.windowTitle_ + "': " + e.what() };
        }
    }

    if (!EnumWindows(
            [](HWND hwnd, LPARAM /*lParam*/) -> BOOL {
                onAddWindow(hwnd);
                return TRUE;
            },
            0)) {
        ERROR_PRINTF("could not list windows: EnumWindows() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    return {};
}

void stop() noexcept
{
    DEBUG_PRINTF("stopping\n");

    hotkeyRestore_.destroy();
    hotkeyRestoreAll_.destroy();
    hotkeyMinimize_.destroy();
    hotkeyMinimizeAll_.destroy();
    hotkeyMenu_.destroy();
}

bool windowShouldAutoTray(HWND hwnd, TrayEvent trayEvent, MinimizePersistence * minimizePersistence)
{
    const WindowInfo windowInfo(hwnd);
    DEBUG_PRINTF("\texecutable: %s\n", windowInfo.executable().c_str());
    DEBUG_PRINTF("\ttitle: %s\n", windowInfo.title().c_str());
    DEBUG_PRINTF("\tclass: %s\n", windowInfo.className().c_str());

    for (const Settings::AutoTray & autoTray : settings_.autoTrays_) {
        bool classMatch = false;
        if (autoTray.windowClass_.empty()) {
            // DEBUG_PRINTF("\twindow class match (empty)\n");
            classMatch = true;
        } else {
            if (autoTray.windowClass_ == windowInfo.className()) {
                // DEBUG_PRINTF("\twindow class '%s' match\n", autoTray.windowClass_.c_str());
                classMatch = true;
            }
        }
        if (!classMatch) {
            DEBUG_PRINTF("\twindow class '%s' does not match\n", autoTray.windowClass_.c_str());
            continue;
        }

        bool executableMatch = false;
        if (autoTray.executable_.empty()) {
            // DEBUG_PRINTF("\texecutable match (empty)\n");
            executableMatch = true;
        } else {
            if (StringUtility::toLower(autoTray.executable_) == StringUtility::toLower(windowInfo.executable())) {
                // DEBUG_PRINTF("\texecutable '%s' match\n", autoTray.executable_.c_str());
                executableMatch = true;
            }
        }
        if (!executableMatch) {
            DEBUG_PRINTF("\texecutable '%s' does not match\n", autoTray.executable_.c_str());
            continue;
        }

        bool titleMatch = false;
        if (autoTray.windowTitle_.empty()) {
            // DEBUG_PRINTF("\twindow title match (empty)\n");
            titleMatch = true;
        } else {
            try {
                const std::regex re(autoTray.windowTitle_);
                if (std::regex_match(windowInfo.title(), re)) {
                    // DEBUG_PRINTF("\twindow title '%s' match\n", autoTray.windowTitle_.c_str());
                    titleMatch = true;
                }
            } catch (const std::regex_error & e) {
                WARNING_PRINTF("regex error: %s\n", e.what());
            }
        }
        if (!titleMatch) {
            DEBUG_PRINTF("\twindow title '%s' does not match\n", autoTray.windowTitle_.c_str());
            continue;
        }

        DEBUG_PRINTF("\tauto-tray ID match\n");

        bool shouldAutoTray = false;
        switch (trayEvent) {
            case TrayEvent::Open: shouldAutoTray = trayEventIncludesOpen(autoTray.trayEvent_); break;
            case TrayEvent::Minimize: shouldAutoTray = trayEventIncludesMinimize(autoTray.trayEvent_); break;
            case TrayEvent::OpenAndMinimize: shouldAutoTray = (autoTray.trayEvent_ != TrayEvent::None); break;
            case TrayEvent::None:
            default: {
                ERROR_PRINTF("invalid auto-tray action\n");
                break;
            }
        }

        if (minimizePersistence) {
            *minimizePersistence = autoTray.minimizePersistence_;
        }

        DEBUG_PRINTF("\tshould auto-tray: %s\n", StringUtility::boolToCString(shouldAutoTray));
        return shouldAutoTray;
    }

    DEBUG_PRINTF("\tno auto-tray match\n");
    return false;
}

void minimizeAllWindows()
{
    std::vector<HWND> windowsToMinimize;

    WindowTracker::enumerate([&windowsToMinimize](const WindowTracker::Item & item) {
        if (item.visible_ && !item.minimized_) {
            DEBUG_PRINTF("minimizing window: %#x\n", item.hwnd_);
            windowsToMinimize.push_back(item.hwnd_);
        }
        return true;
    });

    for (HWND hwnd : windowsToMinimize) {
        minimizeWindow(hwnd, MinimizePersistence::None);
    }
}

void minimizeWindow(HWND hwnd, MinimizePersistence minimizePersistence)
{
    WindowTracker::minimize(hwnd, settings_.minimizePlacement_, minimizePersistence);
}

void restoreAllWindows()
{
    std::vector<HWND> windowsToRestore;

    WindowTracker::reverseEnumerate([&windowsToRestore](const WindowTracker::Item & item) {
        if (item.minimized_) {
            DEBUG_PRINTF("restoring window: %#x\n", item.hwnd_);
            windowsToRestore.push_back(item.hwnd_);
        }
        return true;
    });

    for (HWND hwnd : windowsToRestore) {
        WindowTracker::restore(hwnd);
    }
}

void restoreWindow(HWND hwnd)
{
    WindowTracker::restore(hwnd);
}

void restoreLastWindow()
{
    HWND hwnd = nullptr;

    WindowTracker::reverseEnumerate([&hwnd](const WindowTracker::Item & item) {
        if (item.minimized_) {
            DEBUG_PRINTF("restoring last minimized window: %#x\n", item.hwnd_);
            hwnd = item.hwnd_;
            return false; // stop enumerating
        }
        return true;
    });

    if (hwnd) {
        WindowTracker::restore(hwnd);
    }
}

void onAddWindow(HWND hwnd)
{
    DEBUG_PRINTF("added window: %#x\n", hwnd);

    if (WindowTracker::windowAdded(hwnd)) {
        MinimizePersistence minimizePersistence = MinimizePersistence::None;
        if (windowShouldAutoTray(hwnd, TrayEvent::Open, &minimizePersistence)) {
            if (modifiersActive(modifiersOverride_)) {
                DEBUG_PRINTF("\tmodifier active, not minimizing\n");
            } else {
                DEBUG_PRINTF("\tminimizing\n");
                minimizeWindow(hwnd, minimizePersistence);
            }
        }
    }
}

void onMinimizeEvent(
    HWINEVENTHOOK /* hwineventhook */,
    DWORD event,
    HWND hwnd,
    LONG /* idObject */,
    LONG /* idChild */,
    DWORD /* dwEventThread */,
    DWORD /* dwmsEventTime */)
{
    if (event != EVENT_SYSTEM_MINIMIZESTART) {
        WARNING_PRINTF("unexpected non-minimize event %#x\n", event);
        return;
    }

    if (!isWindowUserVisible(hwnd)) {
        DEBUG_PRINTF("ignoring invisible window: %#x\n", hwnd);
        return;
    }

    DEBUG_PRINTF("minimize start: hwnd %#x\n", hwnd);
    MinimizePersistence minimizePersistence = MinimizePersistence::None;
    if (!windowShouldAutoTray(hwnd, TrayEvent::Minimize, &minimizePersistence)) {
        if (modifiersActive(modifiersOverride_)) {
            DEBUG_PRINTF("\tmodifiers active, minimizing\n");
            minimizeWindow(hwnd, MinimizePersistence::Never);
        }
    } else {
        if (modifiersActive(modifiersOverride_)) {
            DEBUG_PRINTF("\tmodifier active, not minimizing\n");
        } else {
            DEBUG_PRINTF("\tminimizing\n");
            minimizeWindow(hwnd, minimizePersistence);
        }
    }
}

bool readSettingsFromFile(const std::string & fileName, Settings & settings)
{
    DEBUG_PRINTF("Reading settings from file: %s\n", fileName.c_str());

    const std::string writeableDir = getWriteableDir();
    const std::string json = fileRead(pathJoin(writeableDir, fileName));
    if (json.empty()) {
        return false;
    }

    return settings.fromJSON(json);
}

bool writeSettingsToFile(const std::string & fileName, const Settings & settings)
{
    DEBUG_PRINTF("Writing settings to file %s\n", fileName.c_str());

    if (!settings.valid()) {
        ERROR_PRINTF("writing invalid settings\n");
        settings.dump();
    }

    const std::string json = settings.toJSON();
    if (json.empty()) {
        return false;
    }

    const std::string writeableDir = getWriteableDir();
    return fileWrite(pathJoin(writeableDir, fileName), json);
}

void showSettingsDialog()
{
    if (settingsDialogWindow_) {
        WARNING_PRINTF("settings dialog already open, making visible\n");

        // return value intentionally ignored, ShowWindow returns previous visibility
        ShowWindow(settingsDialogWindow_, SW_SHOW);

        // return value intentionally ignored, SetForegroundWindow returns whether brought to foreground
        SetForegroundWindow(settingsDialogWindow_);

        return;
    }

    settingsDialogWindow_ = SettingsDialog::create(appWindow_, settings_, onSettingsDialogComplete);
}

void toggleSettingsDialog()
{
    if (!settingsDialogWindow_) {
        INFO_PRINTF("showing settings dialog\n");
        settingsDialogWindow_ = SettingsDialog::create(appWindow_, settings_, onSettingsDialogComplete);
    } else {
        INFO_PRINTF("hiding settings dialog\n");
        settingsDialogWindow_ = nullptr;
    }
}

void onSettingsDialogComplete(bool success, const Settings & settings)
{
    if (success) {
        if (!settings.valid()) {
            WARNING_PRINTF("invalid settings\n");
            settings_ = settings;
            settings_.dump();

            // restart to trigger error message
            stop();
            const ErrorContext err = start();
            if (!err) {
                ERROR_PRINTF("expected error after restart with invalid settings\n");
            } else {
                errorMessage(err);
                showSettingsDialog();
            }

            return;
        }

        const bool settingsChanged = (settings != settings_);
        const std::string settingsFile = getSettingsFileName();
        if (settingsChanged || !Settings::fileExists(settingsFile)) {
            if (settingsChanged) {
                settings_ = settings;
                DEBUG_PRINTF("got updated settings from dialog:\n");
                settings_.normalize();
                settings_.dump();

                // restart to apply new settings
                stop();
                const ErrorContext err = start();
                if (err) {
                    errorMessage(err);
                    showSettingsDialog();
                    return;
                }
            }

            settings_.normalize();
            if (!writeSettingsToFile(settingsFile, settings_)) {
                errorMessage(ErrorContext(IDS_ERROR_SAVE_SETTINGS, settingsFile));
            } else {
                DEBUG_PRINTF("wrote settings to %s\n", settingsFile.c_str());
            }

            if (settingsChanged) {
                updateStartWithWindowsShortcut();

                WindowTracker::updateMinimizePlacement(settings_.minimizePlacement_);
            }
        }
    }

    settingsDialogWindow_.destroy();
}

std::string getSettingsFileName()
{
    return std::string(APP_NAME) + ".json";
}

std::string getStartupShortcutFullPath()
{
    const std::string startupDir = getStartupDir();
    return pathJoin(startupDir, APP_NAME ".lnk");
}

void updateStartWithWindowsShortcut()
{
    const std::string startupShortcutFullPath = getStartupShortcutFullPath();
    if (settings_.startWithWindows_) {
        if (fileExists(startupShortcutFullPath)) {
            DEBUG_PRINTF("not updating, startup link already exists: %s\n", startupShortcutFullPath.c_str());
        } else {
            const std::string exeFullPath = getExecutableFullPath();
            if (!createShortcut(startupShortcutFullPath, exeFullPath)) {
                WARNING_PRINTF("failed to create startup link: %s\n", startupShortcutFullPath.c_str());
            }
        }
    } else {
        if (!fileExists(startupShortcutFullPath)) {
            DEBUG_PRINTF("not updating, startup link already does not exist: %s\n", startupShortcutFullPath.c_str());
        } else {
            if (!fileDelete(startupShortcutFullPath)) {
                WARNING_PRINTF("failed to delete startup link: %s\n", startupShortcutFullPath.c_str());
            }
        }
    }
}

} // anonymous namespace
