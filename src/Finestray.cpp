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
#include "Finestray.h"
#include "AppName.h"
#include "BitmapHandleWrapper.h"
#include "COMLibraryWrapper.h"
#include "CommandLine.h"
#include "DebugPrint.h"
#include "File.h"
#include "Hotkey.h"
#include "MenuHandleWrapper.h"
#include "Resource.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "StringUtility.h"
#include "TrayIcon.h"
#include "TrayWindow.h"
#include "WinEventHookHandleWrapper.h"
#include "WindowList.h"

// Windows
#include <CommCtrl.h>
#include <Psapi.h>
#include <WinUser.h>
#include <Windows.h>

// Standard library
#include <regex>
#include <set>

static constexpr WORD IDM_APP = 0x1001;
static constexpr WORD IDM_SETTINGS = 0x1002;
static constexpr WORD IDM_ABOUT = 0x1003;
static constexpr WORD IDM_EXIT = 0x1004;
static constexpr WORD IDM_TRAYWINDOW_BASE = 0x1005;

enum class HotkeyID
{
    Minimize = 1,
    Restore
};

static LRESULT wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static int start();
static void stop();
static bool modifiersActive(UINT modifiers);
static bool isAutoTrayWindow(HWND hwnd);
static void onAddWindow(HWND hwnd);
static void onRemoveWindow(HWND hwnd);
static void onMinimizeEvent(
    HWINEVENTHOOK hwineventhook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime);
static void onSettingsDialogComplete(bool success, const Settings & settings);
static bool showContextMenu(HWND hwnd);
static void updateStartWithWindows();
static HBITMAP getResourceBitmap(unsigned int id);
static void replaceBitmapColor(HBITMAP hbmp, uint32_t oldColor, uint32_t newColor);

static HWND hwnd_;
static TrayIcon trayIcon_;
static HWND settingsDialogHwnd_;
static Settings settings_;
static std::set<HWND> autoTrayedWindows_;
static Hotkey hotkeyMinimize_;
static Hotkey hotkeyRestore_;
static UINT modifiersOverride_;

int WINAPI wWinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE prevHinstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    // unused
    (void)prevHinstance;
    (void)pCmdLine;

    HWND oldHwnd = FindWindowA(APP_NAME, nullptr);
    if (oldHwnd) {
        DEBUG_PRINTF("already running\n");
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

    // get command line args
    CommandLine commandLine;
    if (!commandLine.parse()) {
        errorMessage(IDS_ERROR_COMMAND_LINE);
        return IDS_ERROR_COMMAND_LINE;
    }

    if (commandLine.getArgc() > 1) {
        // get settings from command line
        bool parsed = settings_.parseCommandLine(commandLine.getArgc(), commandLine.getArgv());
        if (!parsed) {
            errorMessage(IDS_ERROR_COMMAND_LINE);
            return IDS_ERROR_COMMAND_LINE;
        }
    } else {
        // get settings from file
        std::string exePath = getExecutablePath();
        std::string fileName = pathJoin(exePath, std::string(APP_NAME) + ".json");
        if (settings_.readFromFile(fileName)) {
            DEBUG_PRINTF("read settings from %s\n", fileName.c_str());
        } else {
            if (fileExists(fileName)) {
                errorMessage(IDS_ERROR_LOAD_SETTINGS);
                return IDS_ERROR_LOAD_SETTINGS;
            }
        }
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
        DEBUG_PRINTF("could not create window class, RegisterClassExA() s", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_REGISTER_WINDOW_CLASS);
        return IDS_ERROR_REGISTER_WINDOW_CLASS;
    }

    // create the window
    HWND hwnd = CreateWindowA(
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
    if (!hwnd) {
        DEBUG_PRINTF("could not create window, CreateWindowA() failed: %s", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_WINDOW);
        return IDS_ERROR_CREATE_WINDOW;
    }

    hwnd_ = hwnd;

    // we intentionally don't show the window
    // ShowWindow(hwnd, nCmdShow);
    (void)nCmdShow;

    // create a tray icon for the app
    if (!trayIcon_.create(hwnd, WM_TRAYWINDOW, hicon)) {
        errorMessage(IDS_ERROR_CREATE_TRAY_ICON);
        return IDS_ERROR_CREATE_TRAY_ICON;
    }

    // monitor minimize events
    WinEventHookHandleWrapper winEventHook(SetWinEventHook(
        EVENT_SYSTEM_MINIMIZESTART,
        EVENT_SYSTEM_MINIMIZESTART,
        nullptr,
        onMinimizeEvent,
        0,
        0,
        WINEVENT_OUTOFCONTEXT));
    if (!winEventHook) {
        DEBUG_PRINTF(
            "failed to hook win event %#x, SetWinEventHook() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_REGISTER_EVENTHOOK);
        return IDS_ERROR_REGISTER_EVENTHOOK;
    }

    int err = start();
    if (err) {
        errorMessage(err);
        settingsDialogHwnd_ = SettingsDialog::create(hwnd_, settings_, onSettingsDialogComplete);
    }

    // run the message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        // needed to have working tab stops in the dialog
        if (settingsDialogHwnd_ && IsDialogMessageA(settingsDialogHwnd_, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (!UnhookWinEvent(winEventHook)) {
        DEBUG_PRINTF(
            "failed to unhook win event %#x, UnhookWinEvent() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
    }

    trayIcon_.destroy();

    stop();

    if (!DestroyWindow(hwnd)) {
        DEBUG_PRINTF(
            "failed to destroy window %#x, DestroyWindow() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
    }

    return 0;
}

LRESULT wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT taskbarCreatedMessage;

    switch (uMsg) {
        // command from context menu
        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            switch (id) {
                case IDM_APP: {
                    break;
                }

                case IDM_SETTINGS: {
                    if (!settingsDialogHwnd_) {
                        settingsDialogHwnd_ = SettingsDialog::create(hwnd_, settings_, onSettingsDialogComplete);
                    }
                    break;
                }

                // about dialog
                case IDM_ABOUT: {
                    const std::string & aboutTextStr = getResourceString(IDS_ABOUT_TEXT);
                    const std::string & aboutCaptionStr = getResourceString(IDS_ABOUT_CAPTION);
                    if (!MessageBoxA(hwnd, aboutTextStr.c_str(), aboutCaptionStr.c_str(), MB_OK | MB_ICONINFORMATION)) {
                        DEBUG_PRINTF(
                            "could not create about dialog, MessageBoxA() failed: %s\n",
                            StringUtility::lastErrorString().c_str());
                    }
                    break;
                }

                // exit the app
                case IDM_EXIT: {
                    PostQuitMessage(0);
                    break;
                }

                default: {
                    if ((id >= IDM_TRAYWINDOW_BASE) && (id < (IDM_TRAYWINDOW_BASE + autoTrayedWindows_.size()))) {
                        unsigned int index = id - IDM_TRAYWINDOW_BASE;
                        unsigned int count = 0;
                        for (HWND autoTrayedWindow : autoTrayedWindows_) {
                            if (count == index) {
                                TrayWindow::restore(autoTrayedWindow);
                                break;
                            }
                            ++count;
                        }
                    }
                }
            }
            break;
        }

        case WM_CREATE: {
            // get the message id to be notified when taskbar is (re-)created
            taskbarCreatedMessage = RegisterWindowMessage(TEXT("TaskbarCreated"));
            break;
        }

        case WM_DESTROY: {
            // if there are any minimized windows, restore them
            TrayWindow::restoreAll();

            // exit
            PostQuitMessage(0);
            return 0;
        }

        // our hotkey was activated
        case WM_HOTKEY: {
            HotkeyID hkid = (HotkeyID)wParam;
            switch (hkid) {
                case HotkeyID::Minimize: {
                    DEBUG_PRINTF("hotkey minimize\n");
                    // get the foreground window to minimize
                    HWND foregroundHwnd = GetForegroundWindow();
                    if (foregroundHwnd) {
                        // only minimize windows that have a minimize button
                        LONG windowStyle = GetWindowLong(foregroundHwnd, GWL_STYLE);
                        if (windowStyle & WS_MINIMIZEBOX) {
#if !defined(NDEBUG)
                            CHAR text[256];
                            GetWindowTextA(foregroundHwnd, text, sizeof(text) / sizeof(text[0]));
                            DEBUG_PRINTF("\twindow text '%s'\n", text);

                            CHAR className[256];
                            GetClassNameA(foregroundHwnd, className, sizeof(className) / sizeof(className[0]));
                            DEBUG_PRINTF("\twindow class name '%s'\n", className);
#endif

                            TrayWindow::minimize(foregroundHwnd, hwnd);
                        }
                    }
                    break;
                }

                case HotkeyID::Restore: {
                    DEBUG_PRINTF("hotkey restore\n");
                    HWND lastHwnd = TrayWindow::getLast();
                    if (lastHwnd) {
                        TrayWindow::restore(lastHwnd);
                    }
                    break;
                }

                default: {
                    DEBUG_PRINTF("invalid hotkey id %d\n", hkid);
                    break;
                }
            }
        }

        // message from the tray (taskbar) icon
        case WM_TRAYWINDOW: {
            switch ((UINT)lParam) {
                // user activated context menu
                case WM_CONTEXTMENU: {
                    if (!showContextMenu(hwnd)) {
                        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
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
                    HWND hwndTray = TrayWindow::getFromID((UINT)wParam);
                    if (hwndTray) {
                        TrayWindow::restore(hwndTray);
                    } else if (wParam == trayIcon_.id()) {
                        if (!settingsDialogHwnd_) {
                            settingsDialogHwnd_ = SettingsDialog::create(hwnd_, settings_, onSettingsDialogComplete);
                        } else {
                            if (DestroyWindow(settingsDialogHwnd_)) {
                                settingsDialogHwnd_ = nullptr;
                            }
                        }
                    }
                    break;
                }

                default: {
                    DEBUG_PRINTF("unhandled traywindow message %ld\n", lParam);
                    break;
                }
            }
            break;
        }

        case WM_SHOWSETTINGS: {
            if (!settingsDialogHwnd_) {
                settingsDialogHwnd_ = SettingsDialog::create(hwnd_, settings_, onSettingsDialogComplete);
            }
            break;
        }

        default: {
            if (uMsg == taskbarCreatedMessage) {
                TrayWindow::addAll();
            }
            break;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int start()
{
    // register a hotkey that will be used to minimize windows
    UINT vkMinimize = VK_DOWN;
    UINT modifiersMinimize = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.hotkeyMinimize_, vkMinimize, modifiersMinimize)) {
        return IDS_ERROR_REGISTER_HOTKEY;
    }
    if (!vkMinimize || !modifiersMinimize) {
        DEBUG_PRINTF("no hotkey to minimize windows\n");
    } else {
        if (!hotkeyMinimize_.create((INT)HotkeyID::Minimize, hwnd_, vkMinimize, modifiersMinimize | MOD_NOREPEAT)) {
            return IDS_ERROR_REGISTER_HOTKEY;
        }
    }

    // register a hotkey that will be used to restore windows
    UINT vkRestore = VK_UP;
    UINT modifiersRestore = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.hotkeyRestore_, vkRestore, modifiersRestore)) {
        return IDS_ERROR_REGISTER_HOTKEY;
    }
    if (!vkRestore || !modifiersRestore) {
        DEBUG_PRINTF("no hotkey to restore windows\n");
    } else {
        if (!hotkeyRestore_.create((INT)HotkeyID::Restore, hwnd_, vkRestore, modifiersRestore | MOD_NOREPEAT)) {
            return IDS_ERROR_REGISTER_HOTKEY;
        }
    }

    // get modifiers that will be used to override auto-tray
    UINT vkOverride = 0;
    modifiersOverride_ = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
    if (!Hotkey::parse(settings_.modifiersOverride_, vkOverride, modifiersOverride_)) {
        return IDS_ERROR_REGISTER_MODIFIER;
    }
    if (!modifiersOverride_) {
        DEBUG_PRINTF("no override modifiers\n");
    } else if (vkOverride || (modifiersOverride_ & ~(MOD_ALT | MOD_CONTROL | MOD_SHIFT))) {
        DEBUG_PRINTF("invalid override modifiers\n");
        return IDS_ERROR_REGISTER_MODIFIER;
    }

    WindowList::start(hwnd_, settings_.pollInterval_, onAddWindow, onRemoveWindow);

    return 0;
}

void stop()
{
    WindowList::stop();

    hotkeyRestore_.destroy();
    hotkeyMinimize_.destroy();
}

bool modifiersActive(UINT modifiers)
{
    if (!modifiers) {
        return false;
    }

    if (modifiers & ~(MOD_ALT | MOD_CONTROL | MOD_SHIFT)) {
        DEBUG_PRINTF("invalid modifiers: %#x\n", modifiers);
        return false;
    }

    if (modifiers & MOD_ALT) {
        if (!(GetKeyState(VK_MENU) & 0x8000) && !(GetKeyState(VK_LMENU) & 0x8000) && !(GetKeyState(VK_RMENU) & 0x8000)) {
            DEBUG_PRINTF("\talt modifier not down\n");
            return false;
        }
    }

    if (modifiers & MOD_CONTROL) {
        if (!(GetKeyState(VK_MENU) & 0x8000) && !(GetKeyState(VK_LMENU) & 0x8000) && !(GetKeyState(VK_RMENU) & 0x8000)) {
            DEBUG_PRINTF("\tctrl modifier not down\n");
            return false;
        }
    }

    if (modifiers & MOD_SHIFT) {
        if (!(GetKeyState(VK_SHIFT) & 0x8000) && !(GetKeyState(VK_LSHIFT) & 0x8000) &&
            !(GetKeyState(VK_RSHIFT) & 0x8000)) {
            DEBUG_PRINTF("\tshift modifier not down\n");
            return false;
        }
    }

    if (modifiers & MOD_WIN) {
        if (!(GetKeyState(VK_LWIN) & 0x8000) && !(GetKeyState(VK_RWIN) & 0x8000)) {
            DEBUG_PRINTF("\twin modifier not down\n");
            return false;
        }
    }

    return true;
}

bool isAutoTrayWindow(HWND hwnd)
{
    CHAR executable[MAX_PATH];
    DWORD dwProcId = 0;
    if (!GetWindowThreadProcessId(hwnd, &dwProcId)) {
        DEBUG_PRINTF("GetWindowThreadProcessId() failed: %s\n", StringUtility::lastErrorString().c_str());
    } else {
        HANDLE hproc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId);
        if (!hproc) {
            DEBUG_PRINTF("OpenProcess() failed: %s\n", StringUtility::lastErrorString().c_str());
        } else {
            if (!GetModuleFileNameExA((HMODULE)hproc, nullptr, executable, sizeof(executable))) {
                DEBUG_PRINTF("GetModuleFileNameA() failed: %s\n", StringUtility::lastErrorString().c_str());
            } else {
                DEBUG_PRINTF("\texecutable: %s\n", executable);
            }
        }
        CloseHandle(hproc);
    }

    CHAR windowText[128];
    if (!GetWindowTextA(hwnd, windowText, sizeof(windowText))) {
        // DEBUG_PRINTF("failed to get window text %#x, GetWindowTextA() failed: %s\n", hwnd,
        // StringUtility::lastErrorString().c_str());
    } else {
        DEBUG_PRINTF("\ttitle: %s\n", windowText);
    }

    CHAR className[1024];
    if (!GetClassNameA(hwnd, className, sizeof(className))) {
        DEBUG_PRINTF(
            "failed to get window class name %#x, GetClassNameA() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
    } else {
        DEBUG_PRINTF("\tclass: %s\n", className);
    }

    for (const Settings::AutoTray & autoTray : settings_.autoTrays_) {
        bool executableMatch = false;
        if (autoTray.executable_.empty()) {
            // DEBUG_PRINTF("\texecutable match (empty)\n");
            executableMatch = true;
        } else {
            if (StringUtility::toLower(autoTray.executable_) == StringUtility::toLower(executable)) {
                // DEBUG_PRINTF("\texecutable %s match\n", autoTray.executable_.c_str());
                executableMatch = true;
            }
        }

        bool classMatch = false;
        if (autoTray.windowClass_.empty()) {
            // DEBUG_PRINTF("\twindow class match (empty)\n");
            classMatch = true;
        } else {
            if (autoTray.windowClass_ == className) {
                // DEBUG_PRINTF("\twindow class %s match\n", autoTray.windowClass_.c_str());
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
                // DEBUG_PRINTF("\twindow title %s match\n", autoTray.windowTitle_.c_str());
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

    if (isAutoTrayWindow(hwnd)) {
        if (modifiersActive(modifiersOverride_)) {
            DEBUG_PRINTF("\tmodifier active, not minimizing\n");
        } else {
            DEBUG_PRINTF("\tminimizing\n");
            TrayWindow::minimize(hwnd, hwnd_);
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

    if (!TrayWindow::exists(hwnd)) {
        DEBUG_PRINTF("\tcleaning up\n");
        autoTrayedWindows_.erase(hwnd);
    }
}

void onMinimizeEvent(
    HWINEVENTHOOK /* hwineventhook */,
    DWORD event,
    HWND hwnd,
    LONG /*idObject*/,
    LONG /*idChild*/,
    DWORD /*dwEventThread*/,
    DWORD /*dwmsEventTime*/)
{
    if (event != EVENT_SYSTEM_MINIMIZESTART) {
        DEBUG_PRINTF("unexpected non-minimize event %#x\n", event);
        return;
    }

    DEBUG_PRINTF("minimize start: hwnd %#x\n", hwnd);
    if (!isAutoTrayWindow(hwnd)) {
        if (modifiersActive(modifiersOverride_)) {
            DEBUG_PRINTF("\tmodifiers active, minimizing\n");
            TrayWindow::minimize(hwnd, hwnd_);
        }
    } else {
        if (modifiersActive(modifiersOverride_)) {
            DEBUG_PRINTF("\tmodifier active, not minimizing\n");
        } else {
            DEBUG_PRINTF("\tminimizing\n");
            TrayWindow::minimize(hwnd, hwnd_);
        }
    }
}

void onSettingsDialogComplete(bool success, const Settings & settings)
{
    if (success && (settings != settings_)) {
        settings_ = settings;
        DEBUG_PRINTF("got updated settings from dialog:\n");
        settings_.normalize();
        settings_.dump();

        // restart to apply new settings
        stop();
        int err = start();
        if (err) {
            errorMessage(err);
            settingsDialogHwnd_ = SettingsDialog::create(hwnd_, settings_, onSettingsDialogComplete);
            return;
        }

        std::string fileName(std::string(APP_NAME) + ".json");
        if (!settings_.writeToFile(fileName)) {
            errorMessage(IDS_ERROR_SAVE_SETTINGS);
        } else {
            DEBUG_PRINTF("wrote settings to %s\n", fileName.c_str());
        }

        updateStartWithWindows();
    }

    settingsDialogHwnd_ = nullptr;
}

bool showContextMenu(HWND hwnd)
{
    // create popup menu
    MenuHandleWrapper menu(CreatePopupMenu());
    if (!menu) {
        DEBUG_PRINTF(
            "failed to create context menu, CreatePopupMenu() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // add menu entries
    if (!AppendMenuA(menu, MF_STRING | MF_DISABLED, IDM_APP, APP_NAME)) {
        DEBUG_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }
    if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
        DEBUG_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    if (!autoTrayedWindows_.empty()) {
        unsigned int count = 0;
        for (HWND autoTrayedWindow : autoTrayedWindows_) {
            CHAR title[256];
            GetWindowTextA(autoTrayedWindow, title, sizeof(title) / sizeof(title[0]));
            if (!AppendMenuA(menu, MF_STRING, IDM_TRAYWINDOW_BASE + count, title)) {
                DEBUG_PRINTF(
                    "failed to create menu entry, AppendMenuA() failed: %s\n",
                    StringUtility::lastErrorString().c_str());
                return false;
            }

            HICON hicon = TrayWindow::getIcon(autoTrayedWindow);
            if (hicon) {
                ICONINFO iconinfo;
                if (GetIconInfo(hicon, &iconinfo)) {
                    // FIX - is this a really bad thing to do?
                    uint32_t oldColor = 0;
                    uint32_t menuColor = GetSysColor(COLOR_MENU);
                    uint32_t newColor = RGB(GetBValue(menuColor), GetGValue(menuColor), GetRValue(menuColor));
                    replaceBitmapColor(iconinfo.hbmColor, oldColor, newColor);

                    MENUITEMINFOA menuItemInfo;
                    memset(&menuItemInfo, 0, sizeof(MENUITEMINFOA));
                    menuItemInfo.cbSize = sizeof(MENUITEMINFOA);
                    menuItemInfo.fMask = MIIM_BITMAP;
                    menuItemInfo.hbmpItem = iconinfo.hbmColor;
                    if (!SetMenuItemInfoA(menu, IDM_TRAYWINDOW_BASE + count, FALSE, &menuItemInfo)) {
                        DEBUG_PRINTF(
                            "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                            StringUtility::lastErrorString().c_str());
                        return false;
                    }
                }
            }

            ++count;
        }
        if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
            DEBUG_PRINTF(
                "failed to create menu entry, AppendMenuA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
            return false;
        }
    }

    if (!AppendMenuA(menu, MF_STRING, IDM_SETTINGS, getResourceString(IDS_MENU_SETTINGS).c_str())) {
        DEBUG_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }
    if (!AppendMenuA(menu, MF_STRING, IDM_ABOUT, getResourceString(IDS_MENU_ABOUT).c_str())) {
        DEBUG_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }
    if (!AppendMenuA(menu, MF_STRING, IDM_EXIT, getResourceString(IDS_MENU_EXIT).c_str())) {
        DEBUG_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    BitmapHandleWrapper appBitmap(getResourceBitmap(IDB_APP));
    BitmapHandleWrapper settingsBitmap(getResourceBitmap(IDB_SETTINGS));
    BitmapHandleWrapper aboutBitmap(getResourceBitmap(IDB_ABOUT));
    BitmapHandleWrapper exitBitmap(getResourceBitmap(IDB_EXIT));

    if (!appBitmap || !settingsBitmap || !aboutBitmap || !exitBitmap) {
        DEBUG_PRINTF("failed to load bitmap: %s\n", StringUtility::lastErrorString().c_str());
    } else {
        uint32_t oldColor = RGB(0xFF, 0xFF, 0xFF);
        uint32_t menuColor = GetSysColor(COLOR_MENU);
        uint32_t newColor = RGB(GetBValue(menuColor), GetGValue(menuColor), GetRValue(menuColor));
        replaceBitmapColor(appBitmap, oldColor, newColor);
        replaceBitmapColor(settingsBitmap, oldColor, newColor);
        replaceBitmapColor(aboutBitmap, oldColor, newColor);
        replaceBitmapColor(exitBitmap, oldColor, newColor);

        MENUITEMINFOA menuItemInfo;
        memset(&menuItemInfo, 0, sizeof(MENUITEMINFOA));
        menuItemInfo.cbSize = sizeof(MENUITEMINFOA);
        menuItemInfo.fMask = MIIM_BITMAP;

        menuItemInfo.hbmpItem = appBitmap;
        if (!SetMenuItemInfoA(menu, IDM_APP, FALSE, &menuItemInfo)) {
            DEBUG_PRINTF(
                "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }

        menuItemInfo.hbmpItem = settingsBitmap;
        if (!SetMenuItemInfoA(menu, IDM_SETTINGS, FALSE, &menuItemInfo)) {
            DEBUG_PRINTF(
                "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }

        menuItemInfo.hbmpItem = aboutBitmap;
        if (!SetMenuItemInfoA(menu, IDM_ABOUT, FALSE, &menuItemInfo)) {
            DEBUG_PRINTF(
                "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }

        menuItemInfo.hbmpItem = exitBitmap;
        if (!SetMenuItemInfoA(menu, IDM_EXIT, FALSE, &menuItemInfo)) {
            DEBUG_PRINTF(
                "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }
    }

    // activate our window
    if (!SetForegroundWindow(hwnd)) {
        DEBUG_PRINTF(
            "failed to activate context menu, SetForegroundWindow() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // get the current mouse position
    POINT point = { 0, 0 };
    if (!GetCursorPos(&point)) {
        DEBUG_PRINTF("failed to get menu position, GetCursorPos() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    // show the popup menu
    if (!TrackPopupMenu(menu, 0, point.x, point.y, 0, hwnd, nullptr)) {
        DEBUG_PRINTF(
            "failed to show context menu, TrackPopupMenu() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // force a task switch to our app
    if (!PostMessage(hwnd, WM_USER, 0, 0)) {
        DEBUG_PRINTF("failed to activate app, PostMessage() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    return true;
}

void updateStartWithWindows()
{
    std::string startupPath = getStartupPath();
    std::string startupShortcutPath = pathJoin(startupPath, APP_NAME ".lnk");
    if (settings_.startWithWindows_) {
        if (fileExists(startupShortcutPath)) {
            DEBUG_PRINTF("not updating, startup link already exists: %s\n", startupShortcutPath.c_str());
        } else {
            std::string exePath = getExecutablePath();
            std::string exeFilename = pathJoin(exePath, APP_NAME ".exe");
            if (!createShortcut(startupShortcutPath, exeFilename)) {
                DEBUG_PRINTF("failed to create startup link: %s\n", startupShortcutPath.c_str());
            }
        }
    } else {
        if (!fileExists(startupShortcutPath)) {
            DEBUG_PRINTF("not updating, startup link already does not exist: %s\n", startupShortcutPath.c_str());
        } else {
            if (!deleteFile(startupShortcutPath)) {
                DEBUG_PRINTF("failed to delete startup link: %s\n", startupShortcutPath.c_str());
            }
        }
    }
}

std::string getResourceString(unsigned int id)
{
    HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(nullptr);

    std::string str;
    str.resize(256);
    if (!LoadStringA(hinstance, id, &str[0], (int)str.size())) {
        DEBUG_PRINTF(
            "failed to load resources string %u, LoadStringA() failed: %s\n",
            id,
            StringUtility::lastErrorString().c_str());
        str = "Error ID: " + std::to_string(id);
    }

    return str;
}

HBITMAP getResourceBitmap(unsigned int id)
{
    HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(nullptr);
    HBITMAP bitmap = (HBITMAP)LoadImageA(hinstance, MAKEINTRESOURCEA(id), IMAGE_BITMAP, 0, 0, 0);
    if (!bitmap) {
        DEBUG_PRINTF(
            "failed to load resources bitmap %u, LoadImage() failed: %s\n",
            id,
            StringUtility::lastErrorString().c_str());
    }

    return bitmap;
}

void replaceBitmapColor(HBITMAP hbmp, uint32_t oldColor, uint32_t newColor)
{
    if (!hbmp) {
        return;
    }

    HDC hdc = ::GetDC(HWND_DESKTOP);
    if (!hdc) {
        DEBUG_PRINTF("failed to get desktop DC, GetDC() failed: %s\n", StringUtility::lastErrorString().c_str());
        return;
    }

    BITMAP bitmap;
    memset(&bitmap, 0, sizeof(bitmap));
    if (!GetObject(hbmp, sizeof(bitmap), &bitmap)) {
        DEBUG_PRINTF("failed to get bitmap object, GetObject() failed: %s\n", StringUtility::lastErrorString().c_str());
        ::ReleaseDC(HWND_DESKTOP, hdc);
        return;
    }

    BITMAPINFO bitmapInfo;
    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = bitmap.bmWidth;
    bitmapInfo.bmiHeader.biHeight = bitmap.bmHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;

    std::vector<uint32_t> pixels(bitmap.bmWidth * bitmap.bmHeight);
    if (!GetDIBits(hdc, hbmp, 0, bitmap.bmHeight, &pixels[0], &bitmapInfo, DIB_RGB_COLORS)) {
        DEBUG_PRINTF("failed to get bitmap bits, GetDIBits() failed: %s\n", StringUtility::lastErrorString().c_str());
        ::ReleaseDC(HWND_DESKTOP, hdc);
        return;
    }

    for (uint32_t & pixelColor : pixels) {
        if (pixelColor == oldColor) {
            pixelColor = newColor;
        }
    }

    if (!SetDIBits(hdc, hbmp, 0, bitmap.bmHeight, &pixels[0], &bitmapInfo, DIB_RGB_COLORS)) {
        DEBUG_PRINTF("failed to set bitmap bits, SetDIBits() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    ::ReleaseDC(HWND_DESKTOP, hdc);
}

void errorMessage(unsigned int id)
{
    const std::string & err = getResourceString(id);
    DEBUG_PRINTF("error: %s\n", err.c_str());
    if (!MessageBoxA(nullptr, err.c_str(), APP_NAME, MB_OK | MB_ICONERROR)) {
        DEBUG_PRINTF(
            "failed to display error message %u, MessageBoxA() failed\n",
            id,
            StringUtility::lastErrorString().c_str());
    }
}
