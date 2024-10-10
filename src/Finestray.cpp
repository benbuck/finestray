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
#include "CommandLine.h"
#include "DebugPrint.h"
#include "File.h"
#include "Hotkey.h"
#include "Resource.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "StringUtility.h"
#include "TrayIcon.h"
#include "TrayWindow.h"
#include "WindowList.h"

// Windows
#include <CommCtrl.h>
#include <Psapi.h>
#include <Windows.h>

// Standard library
#include <regex>
#include <set>

static constexpr WORD IDM_SETTINGS = 0x1003;
static constexpr WORD IDM_ABOUT = 0x1004;
static constexpr WORD IDM_EXIT = 0x1005;

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
    HWINEVENTHOOK hook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime);
static void onSettingsDialogComplete(bool success, const Settings & settings);
static bool showContextMenu(HWND hwnd);
static void updateStartWithWindows();

static HWND hwnd_;
static TrayIcon trayIcon_;
static HWND settingsDialog_;
static Settings settings_;
static std::set<HWND> autoTrayedWindows_;
static Hotkey hotkeyMinimize_;
static Hotkey hotkeyRestore_;
static UINT modifiersOverride_;

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    // unused
    (void)hPrevInstance;
    (void)pCmdLine;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        CoUninitialize();
        errorMessage(IDS_ERROR_INIT_COM);
        return IDS_ERROR_INIT_COM;
    }

    INITCOMMONCONTROLSEX initCommonControlsEx;
    initCommonControlsEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    initCommonControlsEx.dwICC = ICC_LISTVIEW_CLASSES;
    if (!InitCommonControlsEx(&initCommonControlsEx)) {
        errorMessage(IDS_ERROR_INIT_COMMON_CONTROLS);
        return IDS_ERROR_INIT_COMMON_CONTROLS;
    }

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

    // get settings from command line
    CommandLine commandLine;
    if (!commandLine.parse()) {
        errorMessage(IDS_ERROR_COMMAND_LINE);
        return IDS_ERROR_COMMAND_LINE;
    }
    bool parsed = settings_.parseCommandLine(commandLine.getArgc(), commandLine.getArgv());
    if (!parsed) {
        errorMessage(IDS_ERROR_COMMAND_LINE);
        return IDS_ERROR_COMMAND_LINE;
    }

    DEBUG_PRINTF("final settings:\n");
    settings_.dump();

    updateStartWithWindows();

    HICON icon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FINESTRAY));

    // create the window class
    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = APP_NAME;
    wc.hIcon = icon;
    wc.hIconSm = icon;
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
        hInstance, // instance
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
    if (!trayIcon_.create(hwnd, WM_TRAYWINDOW, icon)) {
        errorMessage(IDS_ERROR_CREATE_TRAY_ICON);
        return IDS_ERROR_CREATE_TRAY_ICON;
    }

    // monitor minimize events
    HWINEVENTHOOK hWinEventHook = SetWinEventHook(
        EVENT_SYSTEM_MINIMIZESTART,
        EVENT_SYSTEM_MINIMIZESTART,
        nullptr,
        onMinimizeEvent,
        0,
        0,
        WINEVENT_OUTOFCONTEXT);
    if (!hWinEventHook) {
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
        settingsDialog_ = SettingsDialog::create(hwnd_, settings_, onSettingsDialogComplete);
    }

    // run the message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        // needed to have working tab stops in the dialog
        if (settingsDialog_ && IsDialogMessageA(settingsDialog_, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (!UnhookWinEvent(hWinEventHook)) {
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

    CoUninitialize();

    return 0;
}

LRESULT wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT taskbarCreatedMessage;

    switch (uMsg) {
        // command from context menu
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDM_SETTINGS: {
                    if (!settingsDialog_) {
                        settingsDialog_ = SettingsDialog::create(hwnd_, settings_, onSettingsDialogComplete);
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
                    HWND hwndFg = GetForegroundWindow();
                    if (hwndFg) {
                        // only minimize windows that have a minimize button
                        LONG windowStyle = GetWindowLong(hwndFg, GWL_STYLE);
                        if (windowStyle & WS_MINIMIZEBOX) {
#if !defined(NDEBUG)
                            CHAR text[256];
                            GetWindowTextA(hwndFg, text, sizeof(text) / sizeof(text[0]));
                            DEBUG_PRINTF("\twindow text '%s'\n", text);

                            CHAR className[256];
                            GetClassNameA(hwndFg, className, sizeof(className) / sizeof(className[0]));
                            DEBUG_PRINTF("\twindow class name '%s'\n", className);
#endif

                            TrayWindow::minimize(hwndFg, hwnd);
                        }
                    }
                    break;
                }

                case HotkeyID::Restore: {
                    DEBUG_PRINTF("hotkey restore\n");
                    HWND hwndLast = TrayWindow::getLast();
                    if (hwndLast) {
                        TrayWindow::restore(hwndLast);
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
                        if (!settingsDialog_) {
                            settingsDialog_ = SettingsDialog::create(hwnd_, settings_, onSettingsDialogComplete);
                        } else {
                            if (DestroyWindow(settingsDialog_)) {
                                settingsDialog_ = nullptr;
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
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId);
        if (!hProc) {
            DEBUG_PRINTF("OpenProcess() failed: %s\n", StringUtility::lastErrorString().c_str());
        } else {
            if (!GetModuleFileNameExA((HMODULE)hProc, nullptr, executable, sizeof(executable))) {
                DEBUG_PRINTF("GetModuleFileNameA() failed: %s\n", StringUtility::lastErrorString().c_str());
            } else {
                DEBUG_PRINTF("\texecutable: %s\n", executable);
            }
        }
        CloseHandle(hProc);
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
    HWINEVENTHOOK /*hook*/,
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
    if (success) {
        settings_ = settings;
        DEBUG_PRINTF("got updated settings from dialog:\n");
        settings_.normalize();
        settings_.dump();

        // restart to apply new settings
        stop();
        int err = start();
        if (err) {
            errorMessage(err);
            settingsDialog_ = SettingsDialog::create(hwnd_, settings_, onSettingsDialogComplete);
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

    settingsDialog_ = nullptr;
}

bool showContextMenu(HWND hwnd)
{
    // create popup menu
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        DEBUG_PRINTF(
            "failed to create context menu, CreatePopupMenu() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // add menu entries
    if (!AppendMenuA(menu, MF_STRING | MF_DISABLED, 0, APP_NAME)) {
        DEBUG_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }
    if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
        DEBUG_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
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
        if (!DestroyMenu(menu)) {
            DEBUG_PRINTF("failed to destroy menu: %#x\n", menu);
        }
        return false;
    }

    // force a task switch to our app
    if (!PostMessage(hwnd, WM_USER, 0, 0)) {
        DEBUG_PRINTF("failed to activate app, PostMessage() failed: %s\n", StringUtility::lastErrorString().c_str());
        if (!DestroyMenu(menu)) {
            DEBUG_PRINTF("failed to destroy menu: %#x\n", menu);
        }
        return false;
    }

    if (!DestroyMenu(menu)) {
        DEBUG_PRINTF("failed to destroy menu: %#x\n", menu);
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
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);

    std::string str;
    str.resize(256);
    if (!LoadStringA(hInstance, id, &str[0], (int)str.size())) {
        DEBUG_PRINTF(
            "failed to load resources string %u, LoadStringA() failed: %u\n",
            id,
            StringUtility::lastErrorString().c_str());
        str = "Error ID: " + std::to_string(id);
    }

    return str;
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
