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
#include "Helpers.h"
#include "Hotkey.h"
#include "Log.h"
#include "MenuHandleWrapper.h"
#include "MinimizedWindow.h"
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
#include "WindowList.h"
#include "WindowMessage.h"

// Windows
#include <CommCtrl.h>
#include <Psapi.h>
#include <WinUser.h>
#include <Windows.h>

// Standard library
#include <cassert>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace
{

enum class HotkeyID
{
    Minimize = 1,
    Restore,
    RestoreAll,
    Menu
};

LRESULT wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
ErrorContext start();
void stop();
bool windowShouldAutoTray(HWND hwnd);
void onAddWindow(HWND hwnd);
void onRemoveWindow(HWND hwnd);
void onChangeWindowTitle(HWND hwnd, const std::string & title);
void onChangeVisibility(HWND hwnd, bool visible);
void onMinimizeEvent(
    HWINEVENTHOOK hwineventhook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime);
void onSettingsDialogComplete(bool success, const Settings & settings);
std::string getSettingsFileName();
std::string getStartupShortcutFullPath();
void updateStartWithWindows();

WindowHandleWrapper appWindow_;
TrayIcon trayIcon_;
WindowHandleWrapper settingsDialogWindow_;
bool contextMenuActive_;
Settings settings_;
std::set<HWND> autoTrayedWindows_;
Hotkey hotkeyMinimize_;
Hotkey hotkeyRestore_;
Hotkey hotkeyRestoreAll_;
Hotkey hotkeyMenu_;
UINT modifiersOverride_;
UINT taskbarCreatedMessage_;

} // anonymous namespace

int WINAPI wWinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE prevHinstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    // unused
    (void)prevHinstance;
    (void)pCmdLine;

    // check if already running
    HWND oldHwnd = FindWindowA(APP_NAME, nullptr);
    if (oldHwnd) {
        INFO_PRINTF("already running\n");
        SendMessageA(oldHwnd, WM_SHOWSETTINGS, 0, 0);
        return 0;
    }

    // initialize COM
    COMLibraryWrapper comLibrary;
    if (!comLibrary.initialized()) {
        errorMessage(IDS_ERROR_INIT_COM);
        return IDS_ERROR_INIT_COM;
    }

    // initialize common controls
    INITCOMMONCONTROLSEX initCommonControlsEx;
    initCommonControlsEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    initCommonControlsEx.dwICC = ICC_LISTVIEW_CLASSES;
    if (!InitCommonControlsEx(&initCommonControlsEx)) {
        errorMessage(IDS_ERROR_INIT_COMMON_CONTROLS);
        return IDS_ERROR_INIT_COMMON_CONTROLS;
    }

    // get settings from file
    std::string settingsFile = getSettingsFileName();
    if (settings_.readFromFile(settingsFile)) {
        DEBUG_PRINTF("read settings from %s\n", settingsFile.c_str());
    } else {
        if (Settings::fileExists(settingsFile)) {
            errorMessage(ErrorContext(IDS_ERROR_LOAD_SETTINGS, settingsFile));
            return IDS_ERROR_LOAD_SETTINGS;
        }

        // update start with windows setting to match reality
        std::string startupShortcutFullPath = getStartupShortcutFullPath();
        settings_.startWithWindows_ = fileExists(startupShortcutFullPath);
    }

    DEBUG_PRINTF("final settings:\n");
    settings_.dump();

    updateStartWithWindows();

    HICON hicon = LoadIcon(hinstance, MAKEINTRESOURCE(IDI_FINESTRAY));

    // create the window class
    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hinstance;
    wc.lpszClassName = APP_NAME;
    wc.hIcon = hicon;
    wc.hIconSm = hicon;
    ATOM atom = RegisterClassExA(&wc);
    if (!atom) {
        std::string lastErrorString = StringUtility::lastErrorString();
        ERROR_PRINTF("could not create window class, RegisterClassExA() failed: %s", lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_REGISTER_WINDOW_CLASS, lastErrorString));
        return IDS_ERROR_REGISTER_WINDOW_CLASS;
    }

    // create the window
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
        std::string lastErrorString = StringUtility::lastErrorString();
        ERROR_PRINTF("could not create window, CreateWindowA() failed: %s", lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_CREATE_WINDOW, lastErrorString));
        return IDS_ERROR_CREATE_WINDOW;
    }

    // we intentionally don't show the window
    // ShowWindow(appWindow_, nCmdShow);
    (void)nCmdShow;

    ErrorContext err;

    // create a tray icon for the app
    err = trayIcon_.create(appWindow_, appWindow_, WM_TRAYWINDOW, hicon);
    if (err) {
        errorMessage(err);
        return IDS_ERROR_CREATE_TRAY_ICON;
    }

    // monitor minimize events
    WinEventHookHandleWrapper minimizeEventHook(SetWinEventHook(
        EVENT_SYSTEM_MINIMIZESTART,
        EVENT_SYSTEM_MINIMIZESTART,
        nullptr,
        onMinimizeEvent,
        0,
        0,
        WINEVENT_OUTOFCONTEXT));
    if (!minimizeEventHook) {
        std::string lastErrorString = StringUtility::lastErrorString();
        ERROR_PRINTF(
            "failed to hook minimize win event %#x, SetWinEventHook() failed: %s\n",
            (HWND)appWindow_,
            lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_REGISTER_EVENTHOOK, lastErrorString));
        return IDS_ERROR_REGISTER_EVENTHOOK;
    }

    err = start();
    if (err) {
        errorMessage(err);
        settingsDialogWindow_ = SettingsDialog::create(appWindow_, settings_, onSettingsDialogComplete);
    } else {
        if (!Settings::fileExists(settingsFile)) {
            settingsDialogWindow_ = SettingsDialog::create(appWindow_, settings_, onSettingsDialogComplete);
        }
    }

    // run the message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        // needed to have working tab stops in the dialog
        if (settingsDialogWindow_ && IsDialogMessageA(settingsDialogWindow_, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    minimizeEventHook.destroy();
    trayIcon_.destroy();
    stop();
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
            WORD id = LOWORD(wParam);
            switch (id) {
                // about dialog
                case IDM_APP:
                case IDM_ABOUT: {
                    showAboutDialog(hwnd);
                    break;
                }

                case IDM_RESTORE_ALL: {
                    MinimizedWindow::restoreAll();
                    break;
                }

                case IDM_SETTINGS: {
                    if (!settingsDialogWindow_) {
                        settingsDialogWindow_ = SettingsDialog::create(hwnd, settings_, onSettingsDialogComplete);
                    }
                    break;
                }

                // exit the app
                case IDM_EXIT: {
                    PostQuitMessage(0);
                    break;
                }

                default: {
                    std::vector<HWND> minimizedWindows = MinimizedWindow::getAll();
                    if ((id >= IDM_MINIMIZEDWINDOW_BASE) && (id < (IDM_MINIMIZEDWINDOW_BASE + minimizedWindows.size()))) {
                        unsigned int index = id - IDM_MINIMIZEDWINDOW_BASE;
                        unsigned int count = 0;
                        for (HWND minimizedWindow : minimizedWindows) {
                            if (count == index) {
                                MinimizedWindow::restore(minimizedWindow);
                                break;
                            }
                            ++count;
                        }
                    } else {
                        std::map<HWND, WindowList::WindowData> windowList = WindowList::getAll();
                        if ((id >= IDM_VISIBLEWINDOW_BASE) && (id < (IDM_VISIBLEWINDOW_BASE + windowList.size()))) {
                            unsigned int index = id - IDM_VISIBLEWINDOW_BASE;
                            unsigned int count = 0;
                            for (const std::pair<HWND, WindowList::WindowData> & window : windowList) {
                                if (window.second.visible) {
                                    if (count == index) {
                                        MinimizedWindow::minimize(window.first, hwnd, settings_.minimizePlacement_);
                                        break;
                                    }
                                    ++count;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }

        case WM_CREATE: {
            // get the message id to be notified when taskbar is (re-)created
            taskbarCreatedMessage_ = RegisterWindowMessage(TEXT("TaskbarCreated"));
            break;
        }

        case WM_DESTROY: {
            // if there are any minimized windows, restore them
            MinimizedWindow::restoreAll();

            // exit
            PostQuitMessage(0);
            return 0;
        }

        // our hotkey was activated
        case WM_HOTKEY: {
            HotkeyID hkid = (HotkeyID)wParam;
            switch (hkid) {
                case HotkeyID::Minimize: {
                    INFO_PRINTF("hotkey minimize\n");
                    // get the foreground window to minimize
                    HWND foregroundHwnd = GetForegroundWindow();
                    if (foregroundHwnd) {
                        // only minimize windows that have a minimize button
                        LONG windowStyle = GetWindowLong(foregroundHwnd, GWL_STYLE);
                        if (windowStyle & WS_MINIMIZEBOX) {
#if !defined(NDEBUG)
                            std::string text = getWindowText(foregroundHwnd);
                            DEBUG_PRINTF("\twindow text '%s'\n", text.c_str());

                            std::string className = getWindowClassName(foregroundHwnd);
                            DEBUG_PRINTF("\twindow class name '%s'\n", className.c_str());
#endif

                            MinimizedWindow::minimize(foregroundHwnd, hwnd, settings_.minimizePlacement_);
                        }
                    }
                    break;
                }

                case HotkeyID::Restore: {
                    INFO_PRINTF("hotkey restore\n");
                    HWND lastHwnd = MinimizedWindow::getLast();
                    if (lastHwnd) {
                        MinimizedWindow::restore(lastHwnd);
                    }
                    break;
                }

                case HotkeyID::RestoreAll: {
                    INFO_PRINTF("hotkey restore all\n");
                    MinimizedWindow::restoreAll();
                    break;
                }

                case HotkeyID::Menu: {
                    if (contextMenuActive_) {
                        WARNING_PRINTF("context menu already active, ignoring hotkey\n");
                        break;
                    }
                    if (!showContextMenu(hwnd, settings_.minimizePlacement_, settings_.showWindowsInMenu_)) {
                        errorMessage(IDS_ERROR_CREATE_MENU);
                    }
                    break;
                }

                default: {
                    WARNING_PRINTF("invalid hotkey id %d\n", hkid);
                    break;
                }
            }
        }

        // message from the tray (taskbar) icon
        case WM_TRAYWINDOW: {
            switch ((UINT)lParam) {
                // user activated context menu
                case WM_CONTEXTMENU: {
                    if (contextMenuActive_) {
                        WARNING_PRINTF("context menu already active, ignoring\n");
                        break;
                    }
                    if (!showContextMenu(hwnd, settings_.minimizePlacement_, settings_.showWindowsInMenu_)) {
                        errorMessage(IDS_ERROR_CREATE_MENU);
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
                    HWND hwndTray = MinimizedWindow::getFromID((UINT)wParam);
                    if (hwndTray) {
                        MinimizedWindow::restore(hwndTray);
                    } else if (wParam == trayIcon_.id()) {
                        if (!settingsDialogWindow_) {
                            settingsDialogWindow_ = SettingsDialog::create(hwnd, settings_, onSettingsDialogComplete);
                        } else {
                            settingsDialogWindow_ = nullptr;
                        }
                    }
                    break;
                }

                default: {
                    WARNING_PRINTF("unhandled MinimizedWindow message %ld\n", lParam);
                    break;
                }
            }
            break;
        }

        case WM_SHOWSETTINGS: {
            if (!settingsDialogWindow_) {
                settingsDialogWindow_ = SettingsDialog::create(hwnd, settings_, onSettingsDialogComplete);
            }
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
                MinimizedWindow::addAll(settings_.minimizePlacement_);
            }
            break;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

ErrorContext start()
{
    Log::start(settings_.logToFile_, APP_NAME ".log");

    // register a hotkey that will be used to minimize windows
    UINT vkMinimize = VK_DOWN;
    UINT modifiersMinimize = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.hotkeyMinimize_, vkMinimize, modifiersMinimize)) {
        return ErrorContext(IDS_ERROR_PARSE_HOTKEY, "minimize");
    }
    if (!vkMinimize || !modifiersMinimize) {
        INFO_PRINTF("no hotkey to minimize windows\n");
    } else {
        if (!hotkeyMinimize_.create((INT)HotkeyID::Minimize, appWindow_, vkMinimize, modifiersMinimize | MOD_NOREPEAT)) {
            return ErrorContext(IDS_ERROR_REGISTER_HOTKEY, "minimize");
        }
    }

    // register a hotkey that will be used to restore windows
    UINT vkRestore = VK_UP;
    UINT modifiersRestore = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.hotkeyRestore_, vkRestore, modifiersRestore)) {
        return ErrorContext(IDS_ERROR_PARSE_HOTKEY, "restore");
    }
    if (!vkRestore || !modifiersRestore) {
        INFO_PRINTF("no hotkey to restore windows\n");
    } else {
        if (!hotkeyRestore_.create((INT)HotkeyID::Restore, appWindow_, vkRestore, modifiersRestore | MOD_NOREPEAT)) {
            return ErrorContext(IDS_ERROR_REGISTER_HOTKEY, "restore");
        }
    }

    // register a hotkey that will be used to restore all windows
    UINT vkRestoreAll = VK_LEFT;
    UINT modifiersRestoreAll = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.hotkeyRestoreAll_, vkRestoreAll, modifiersRestoreAll)) {
        return ErrorContext(IDS_ERROR_PARSE_HOTKEY, "restore all");
    }
    if (!vkRestoreAll || !modifiersRestoreAll) {
        INFO_PRINTF("no hotkey to restore all windows\n");
    } else {
        if (!hotkeyRestoreAll_
                 .create((INT)HotkeyID::RestoreAll, appWindow_, vkRestoreAll, modifiersRestoreAll | MOD_NOREPEAT)) {
            return ErrorContext(IDS_ERROR_REGISTER_HOTKEY, "restore all");
        }
    }

    // register a hotkey that will be used to show the context menu
    UINT vkMenu = VK_HOME;
    UINT modifiersMenu = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.hotkeyMenu_, vkMenu, modifiersMenu)) {
        return ErrorContext(IDS_ERROR_PARSE_HOTKEY, "menu");
    }
    if (!vkMenu || !modifiersMenu) {
        INFO_PRINTF("no hotkey to Menu windows\n");
    } else {
        if (!hotkeyMenu_.create((INT)HotkeyID::Menu, appWindow_, vkMenu, modifiersMenu | MOD_NOREPEAT)) {
            return ErrorContext(IDS_ERROR_REGISTER_HOTKEY, "menu");
        }
    }

    // get modifiers that will be used to override auto-tray
    UINT vkOverride = 0;
    modifiersOverride_ = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.modifiersOverride_, vkOverride, modifiersOverride_)) {
        return ErrorContext(IDS_ERROR_PARSE_MODIFIER, "override");
    }
    if (!modifiersOverride_) {
        INFO_PRINTF("no override modifiers\n");
    } else if (vkOverride || (modifiersOverride_ & ~(MOD_ALT | MOD_CONTROL | MOD_SHIFT))) {
        WARNING_PRINTF("invalid override modifiers\n");
        return ErrorContext(IDS_ERROR_REGISTER_MODIFIER, "override");
    }

    WindowList::start(appWindow_, settings_.pollInterval_, onAddWindow, onRemoveWindow, onChangeWindowTitle, onChangeVisibility);

    return ErrorContext();
}

void stop()
{
    WindowList::stop();

    hotkeyRestore_.destroy();
    hotkeyRestoreAll_.destroy();
    hotkeyMinimize_.destroy();
    hotkeyMenu_.destroy();
}

bool windowShouldAutoTray(HWND hwnd)
{
    CHAR executable[MAX_PATH];
    DWORD dwProcId = 0;
    if (!GetWindowThreadProcessId(hwnd, &dwProcId)) {
        WARNING_PRINTF("GetWindowThreadProcessId() failed: %s\n", StringUtility::lastErrorString().c_str());
    } else {
        HANDLE hproc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId);
        if (!hproc) {
            WARNING_PRINTF("OpenProcess() failed: %s\n", StringUtility::lastErrorString().c_str());
        } else {
            if (!GetModuleFileNameExA((HMODULE)hproc, nullptr, executable, sizeof(executable))) {
                WARNING_PRINTF("GetModuleFileNameA() failed: %s\n", StringUtility::lastErrorString().c_str());
            } else {
                DEBUG_PRINTF("\texecutable: %s\n", executable);
            }
        }
        CloseHandle(hproc);
    }

    std::string windowText = getWindowText(hwnd);
    if (!windowText.empty()) {
        DEBUG_PRINTF("\ttitle: %s\n", windowText.c_str());
    }

    std::string className = getWindowClassName(hwnd);
    if (!className.empty()) {
        DEBUG_PRINTF("\tclass: %s\n", className.c_str());
    }

    for (const Settings::AutoTray & autoTray : settings_.autoTrays_) {
        bool executableMatch = false;
        if (autoTray.executable_.empty()) {
            // DEBUG_PRINTF("\texecutable match (empty)\n");
            executableMatch = true;
        } else {
            if (StringUtility::toLower(autoTray.executable_) == StringUtility::toLower(executable)) {
                // DEBUG_PRINTF("\texecutable '%s' match\n", autoTray.executable_.c_str());
                executableMatch = true;
            }
        }

        bool classMatch = false;
        if (autoTray.windowClass_.empty()) {
            // DEBUG_PRINTF("\twindow class match (empty)\n");
            classMatch = true;
        } else {
            if (autoTray.windowClass_ == className) {
                // DEBUG_PRINTF("\twindow class '%s' match\n", autoTray.windowClass_.c_str());
                classMatch = true;
            }
        }

        bool titleMatch = false;
        if (autoTray.windowTitle_.empty()) {
            // DEBUG_PRINTF("\twindow title match (empty)\n");
            titleMatch = true;
        } else {
            std::regex re(autoTray.windowTitle_);
            if (std::regex_match(windowText, re)) {
                // DEBUG_PRINTF("\twindow title '%s' match\n", autoTray.windowTitle_.c_str());
                titleMatch = true;
            }
        }

        if (executableMatch && classMatch && titleMatch) {
            DEBUG_PRINTF("\tauto-tray match\n");
            return true;
        }
    }

    return false;
}

void onAddWindow(HWND hwnd)
{
    DEBUG_PRINTF("added window: %#x\n", hwnd);

    if (autoTrayedWindows_.find(hwnd) != autoTrayedWindows_.end()) {
        DEBUG_PRINTF("\tignoring, previously auto-trayed\n");
        return;
    }

    if (windowShouldAutoTray(hwnd)) {
        if (modifiersActive(modifiersOverride_)) {
            DEBUG_PRINTF("\tmodifier active, not minimizing\n");
        } else {
            DEBUG_PRINTF("\tminimizing\n");
            MinimizedWindow::minimize(hwnd, appWindow_, settings_.minimizePlacement_);
            autoTrayedWindows_.insert(hwnd);
        }
    }
}

void onRemoveWindow(HWND hwnd)
{
    DEBUG_PRINTF("removed window: %#x\n", hwnd);

    auto it = autoTrayedWindows_.find(hwnd);
    if (it == autoTrayedWindows_.end()) {
        DEBUG_PRINTF("\tignoring, not auto-trayed\n");
        return;
    }

    if (!MinimizedWindow::exists(hwnd)) {
        DEBUG_PRINTF("\tcleaning up\n");
        MinimizedWindow::remove(hwnd);
        autoTrayedWindows_.erase(hwnd);
    }
}

void onChangeWindowTitle(HWND hwnd, const std::string & title)
{
    DEBUG_PRINTF("changed window title: %#x\n", hwnd);

    if (MinimizedWindow::exists(hwnd)) {
        DEBUG_PRINTF("\tupdating title\n");
        MinimizedWindow::updateTitle(hwnd, title);
    }
}

void onChangeVisibility(HWND hwnd, bool visible)
{
    DEBUG_PRINTF("changed window visibility: %#x to %s\n", hwnd, StringUtility::boolToCString(visible));

    if (visible && MinimizedWindow::exists(hwnd)) {
        MinimizedWindow::remove(hwnd);
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
    if (!windowShouldAutoTray(hwnd)) {
        if (modifiersActive(modifiersOverride_)) {
            DEBUG_PRINTF("\tmodifiers active, minimizing\n");
            MinimizedWindow::minimize(hwnd, appWindow_, settings_.minimizePlacement_);
        }
    } else {
        if (modifiersActive(modifiersOverride_)) {
            DEBUG_PRINTF("\tmodifier active, not minimizing\n");
        } else {
            DEBUG_PRINTF("\tminimizing\n");
            MinimizedWindow::minimize(hwnd, appWindow_, settings_.minimizePlacement_);
            autoTrayedWindows_.insert(hwnd);
        }
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
            ErrorContext err = start();
            assert(err);
            if (err) {
                errorMessage(err);
                settingsDialogWindow_ = SettingsDialog::create(appWindow_, settings_, onSettingsDialogComplete);
            }

            return;
        }

        bool settingsChanged = (settings != settings_);
        std::string settingsFile = getSettingsFileName();
        if (settingsChanged || !Settings::fileExists(settingsFile)) {
            if (settingsChanged) {
                settings_ = settings;
                DEBUG_PRINTF("got updated settings from dialog:\n");
                settings_.normalize();
                settings_.dump();

                // restart to apply new settings
                stop();
                ErrorContext err = start();
                if (err) {
                    errorMessage(err);
                    settingsDialogWindow_ = SettingsDialog::create(appWindow_, settings_, onSettingsDialogComplete);
                    return;
                }
            }

            if (!settings_.writeToFile(settingsFile)) {
                errorMessage(ErrorContext(IDS_ERROR_SAVE_SETTINGS, settingsFile));
            } else {
                DEBUG_PRINTF("wrote settings to %s\n", settingsFile.c_str());
            }

            if (settingsChanged) {
                updateStartWithWindows();

                MinimizedWindow::updatePlacement(settings_.minimizePlacement_);
            }
        }
    }

    settingsDialogWindow_ = nullptr;
}

std::string getSettingsFileName()
{
    return std::string(APP_NAME) + ".json";
}

std::string getStartupShortcutFullPath()
{
    std::string startupDir = getStartupDir();
    return pathJoin(startupDir, APP_NAME ".lnk");
}

void updateStartWithWindows()
{
    std::string startupShortcutFullPath = getStartupShortcutFullPath();
    if (settings_.startWithWindows_) {
        if (fileExists(startupShortcutFullPath)) {
            DEBUG_PRINTF("not updating, startup link already exists: %s\n", startupShortcutFullPath.c_str());
        } else {
            std::string exeFullPath = getExecutableFullPath();
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
