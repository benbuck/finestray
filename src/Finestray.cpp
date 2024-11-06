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
#include "File.h"
#include "Hotkey.h"
#include "Log.h"
#include "MenuHandleWrapper.h"
#include "MinimizedWindow.h"
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
#include <regex>
#include <set>

namespace
{

constexpr WORD IDM_APP = 0x1001;
constexpr WORD IDM_SETTINGS = 0x1002;
constexpr WORD IDM_ABOUT = 0x1003;
constexpr WORD IDM_EXIT = 0x1004;
constexpr WORD IDM_MINIMIZEDWINDOW_BASE = 0x1005;

enum class HotkeyID
{
    Minimize = 1,
    Restore
};

LRESULT wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int start();
void stop();
bool modifiersActive(UINT modifiers);
bool windowShouldAutoTray(HWND hwnd);
void onAddWindow(HWND hwnd);
void onRemoveWindow(HWND hwnd);
void onChangeWindowTitle(HWND hwnd, const std::string & title);
void onMinimizeEvent(
    HWINEVENTHOOK hwineventhook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime);
void onSettingsDialogComplete(bool success, const Settings & settings);
bool showContextMenu(HWND hwnd);
void updateStartWithWindows();
HBITMAP getResourceBitmap(unsigned int id);
void replaceBitmapColor(HBITMAP hbmp, uint32_t oldColor, uint32_t newColor);

WindowHandleWrapper appWindow_;
TrayIcon trayIcon_;
WindowHandleWrapper settingsDialogWindow_;
Settings settings_;
std::set<HWND> autoTrayedWindows_;
Hotkey hotkeyMinimize_;
Hotkey hotkeyRestore_;
UINT modifiersOverride_;
UINT taskbarCreatedMessage_;
bool aboutDialogOpen_;

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
        ERROR_PRINTF(
            "could not create window class, RegisterClassExA() failed: %s",
            StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_REGISTER_WINDOW_CLASS);
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
        ERROR_PRINTF("could not create window, CreateWindowA() failed: %s", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_WINDOW);
        return IDS_ERROR_CREATE_WINDOW;
    }

    // we intentionally don't show the window
    // ShowWindow(appWindow_, nCmdShow);
    (void)nCmdShow;

    // create a tray icon for the app
    if (!trayIcon_.create(appWindow_, appWindow_, WM_TRAYWINDOW, hicon)) {
        errorMessage(IDS_ERROR_CREATE_TRAY_ICON);
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
        ERROR_PRINTF(
            "failed to hook minimize win event %#x, SetWinEventHook() failed: %s\n",
            (HWND)appWindow_,
            StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_REGISTER_EVENTHOOK);
        return IDS_ERROR_REGISTER_EVENTHOOK;
    }

    int err = start();
    if (err) {
        errorMessage(err);
        settingsDialogWindow_ = SettingsDialog::create(appWindow_, settings_, onSettingsDialogComplete);
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

void showAboutDialog(HWND hwnd)
{
    if (aboutDialogOpen_) {
        return;
    }

    const std::string & aboutTextStr = getResourceString(IDS_ABOUT_TEXT);
    const std::string & aboutCaptionStr = getResourceString(IDS_ABOUT_CAPTION);
    UINT type = MB_OK | MB_ICONINFORMATION | MB_TASKMODAL;

    aboutDialogOpen_ = true;
    int result = MessageBoxA(hwnd, aboutTextStr.c_str(), aboutCaptionStr.c_str(), type);
    aboutDialogOpen_ = false;

    if (!result) {
        WARNING_PRINTF(
            "could not create about dialog, MessageBoxA() failed: %s\n",
            StringUtility::lastErrorString().c_str());
    }
}

std::string getResourceString(unsigned int id)
{
    HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(nullptr);

    std::string str;
    str.resize(256);
    if (!LoadStringA(hinstance, id, &str[0], (int)str.size())) {
        WARNING_PRINTF(
            "failed to load resources string %u, LoadStringA() failed: %s\n",
            id,
            StringUtility::lastErrorString().c_str());
        str = "Error ID: " + std::to_string(id);
    }

    return str;
}

void errorMessage(unsigned int id)
{
    const std::string & err = getResourceString(id);
    ERROR_PRINTF("%s\n", err.c_str());
    if (!MessageBoxA(nullptr, err.c_str(), APP_NAME, MB_OK | MB_ICONERROR)) {
        WARNING_PRINTF(
            "failed to display error message %u, MessageBoxA() failed\n",
            id,
            StringUtility::lastErrorString().c_str());
    }
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
                case IDM_SETTINGS: {
                    if (!settingsDialogWindow_) {
                        settingsDialogWindow_ = SettingsDialog::create(hwnd, settings_, onSettingsDialogComplete);
                    }
                    break;
                }

                // about dialog
                case IDM_APP:
                case IDM_ABOUT: {
                    showAboutDialog(hwnd);
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
                            CHAR text[256];
                            GetWindowTextA(foregroundHwnd, text, sizeof(text) / sizeof(text[0]));
                            DEBUG_PRINTF("\twindow text '%s'\n", text);

                            CHAR className[256];
                            GetClassNameA(foregroundHwnd, className, sizeof(className) / sizeof(className[0]));
                            DEBUG_PRINTF("\twindow class name '%s'\n", className);
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

        default: {
            if (uMsg == taskbarCreatedMessage_) {
                MinimizedWindow::addAll(settings_.minimizePlacement_);
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
        INFO_PRINTF("no hotkey to minimize windows\n");
    } else {
        if (!hotkeyMinimize_.create((INT)HotkeyID::Minimize, appWindow_, vkMinimize, modifiersMinimize | MOD_NOREPEAT)) {
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
        INFO_PRINTF("no hotkey to restore windows\n");
    } else {
        if (!hotkeyRestore_.create((INT)HotkeyID::Restore, appWindow_, vkRestore, modifiersRestore | MOD_NOREPEAT)) {
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
        INFO_PRINTF("no override modifiers\n");
    } else if (vkOverride || (modifiersOverride_ & ~(MOD_ALT | MOD_CONTROL | MOD_SHIFT))) {
        WARNING_PRINTF("invalid override modifiers\n");
        return IDS_ERROR_REGISTER_MODIFIER;
    }

    WindowList::start(appWindow_, settings_.pollInterval_, onAddWindow, onRemoveWindow, onChangeWindowTitle);

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
        WARNING_PRINTF("invalid modifiers: %#x\n", modifiers);
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

    CHAR windowText[128];
    if (!GetWindowTextA(hwnd, windowText, sizeof(windowText)) && (GetLastError() != ERROR_SUCCESS)) {
        // WARNING_PRINTF("failed to get window text %#x, GetWindowTextA() failed: %s\n", hwnd,
        // StringUtility::lastErrorString().c_str());
    } else {
        DEBUG_PRINTF("\ttitle: %s\n", windowText);
    }

    CHAR className[1024];
    if (!GetClassNameA(hwnd, className, sizeof(className))) {
        WARNING_PRINTF(
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
            settingsDialogWindow_ = SettingsDialog::create(appWindow_, settings_, onSettingsDialogComplete);
            return;
        }

        std::string fileName(std::string(APP_NAME) + ".json");
        if (!settings_.writeToFile(fileName)) {
            errorMessage(IDS_ERROR_SAVE_SETTINGS);
        } else {
            DEBUG_PRINTF("wrote settings to %s\n", fileName.c_str());
        }

        updateStartWithWindows();

        MinimizedWindow::updatePlacement(settings_.minimizePlacement_);
    }

    settingsDialogWindow_ = nullptr;
}

bool showContextMenu(HWND hwnd)
{
    // create popup menu
    MenuHandleWrapper menu(CreatePopupMenu());
    if (!menu) {
        WARNING_PRINTF(
            "failed to create context menu, CreatePopupMenu() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // add menu entries
    if (!AppendMenuA(menu, MF_STRING, IDM_APP, APP_NAME)) {
        WARNING_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }
    if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
        WARNING_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    if (minimizePlacementIncludesMenu(settings_.minimizePlacement_)) {
        unsigned int count = 0;
        std::vector<HWND> minimizedWindows = MinimizedWindow::getAll();
        for (HWND minimizedWindow : minimizedWindows) {
            CHAR title[256];
            GetWindowTextA(minimizedWindow, title, sizeof(title) / sizeof(title[0]));
            if (!AppendMenuA(menu, MF_STRING, IDM_MINIMIZEDWINDOW_BASE + count, title)) {
                WARNING_PRINTF(
                    "failed to create menu entry, AppendMenuA() failed: %s\n",
                    StringUtility::lastErrorString().c_str());
                return false;
            }

            HICON hicon = WindowIcon::get(minimizedWindow);
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
                    if (!SetMenuItemInfoA(menu, IDM_MINIMIZEDWINDOW_BASE + count, FALSE, &menuItemInfo)) {
                        WARNING_PRINTF(
                            "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                            StringUtility::lastErrorString().c_str());
                        return false;
                    }
                }
            }

            ++count;
        }
        if (count) {
            if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
                WARNING_PRINTF(
                    "failed to create menu entry, AppendMenuA() failed: %s\n",
                    StringUtility::lastErrorString().c_str());
                return false;
            }
        }
    }

    if (!AppendMenuA(menu, MF_STRING, IDM_SETTINGS, getResourceString(IDS_MENU_SETTINGS).c_str())) {
        WARNING_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }
    if (!AppendMenuA(menu, MF_STRING, IDM_EXIT, getResourceString(IDS_MENU_EXIT).c_str())) {
        WARNING_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    BitmapHandleWrapper appBitmap(getResourceBitmap(IDB_APP));
    BitmapHandleWrapper settingsBitmap(getResourceBitmap(IDB_SETTINGS));
    BitmapHandleWrapper exitBitmap(getResourceBitmap(IDB_EXIT));

    if (!appBitmap || !settingsBitmap || !exitBitmap) {
        WARNING_PRINTF("failed to load bitmap: %s\n", StringUtility::lastErrorString().c_str());
    } else {
        uint32_t oldColor = RGB(0xFF, 0xFF, 0xFF);
        uint32_t menuColor = GetSysColor(COLOR_MENU);
        uint32_t newColor = RGB(GetBValue(menuColor), GetGValue(menuColor), GetRValue(menuColor));
        replaceBitmapColor(appBitmap, oldColor, newColor);
        replaceBitmapColor(settingsBitmap, oldColor, newColor);
        replaceBitmapColor(exitBitmap, oldColor, newColor);

        MENUITEMINFOA menuItemInfo;
        memset(&menuItemInfo, 0, sizeof(MENUITEMINFOA));
        menuItemInfo.cbSize = sizeof(MENUITEMINFOA);
        menuItemInfo.fMask = MIIM_BITMAP;

        menuItemInfo.hbmpItem = appBitmap;
        if (!SetMenuItemInfoA(menu, IDM_APP, FALSE, &menuItemInfo)) {
            WARNING_PRINTF(
                "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }

        menuItemInfo.hbmpItem = settingsBitmap;
        if (!SetMenuItemInfoA(menu, IDM_SETTINGS, FALSE, &menuItemInfo)) {
            WARNING_PRINTF(
                "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }

        menuItemInfo.hbmpItem = exitBitmap;
        if (!SetMenuItemInfoA(menu, IDM_EXIT, FALSE, &menuItemInfo)) {
            WARNING_PRINTF(
                "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }
    }

    // activate our window
    if (!SetForegroundWindow(hwnd)) {
        WARNING_PRINTF(
            "failed to activate context menu, SetForegroundWindow() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // get the current mouse position
    POINT point = { 0, 0 };
    if (!GetCursorPos(&point)) {
        WARNING_PRINTF(
            "failed to get menu position, GetCursorPos() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // show the popup menu
    if (!TrackPopupMenu(menu, 0, point.x, point.y, 0, hwnd, nullptr)) {
        WARNING_PRINTF(
            "failed to show context menu, TrackPopupMenu() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // force a task switch to our app
    if (!PostMessage(hwnd, WM_USER, 0, 0)) {
        WARNING_PRINTF("failed to activate app, PostMessage() failed: %s\n", StringUtility::lastErrorString().c_str());
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
                WARNING_PRINTF("failed to create startup link: %s\n", startupShortcutPath.c_str());
            }
        }
    } else {
        if (!fileExists(startupShortcutPath)) {
            DEBUG_PRINTF("not updating, startup link already does not exist: %s\n", startupShortcutPath.c_str());
        } else {
            if (!deleteFile(startupShortcutPath)) {
                WARNING_PRINTF("failed to delete startup link: %s\n", startupShortcutPath.c_str());
            }
        }
    }
}

HBITMAP getResourceBitmap(unsigned int id)
{
    HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(nullptr);
    HBITMAP bitmap = (HBITMAP)LoadImageA(hinstance, MAKEINTRESOURCEA(id), IMAGE_BITMAP, 0, 0, 0);
    if (!bitmap) {
        WARNING_PRINTF(
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
        WARNING_PRINTF("failed to get desktop DC, GetDC() failed: %s\n", StringUtility::lastErrorString().c_str());
        return;
    }

    BITMAP bitmap;
    memset(&bitmap, 0, sizeof(bitmap));
    if (!GetObject(hbmp, sizeof(bitmap), &bitmap)) {
        WARNING_PRINTF("failed to get bitmap object, GetObject() failed: %s\n", StringUtility::lastErrorString().c_str());
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
        WARNING_PRINTF("failed to get bitmap bits, GetDIBits() failed: %s\n", StringUtility::lastErrorString().c_str());
        ::ReleaseDC(HWND_DESKTOP, hdc);
        return;
    }

    for (uint32_t & pixelColor : pixels) {
        if (pixelColor == oldColor) {
            pixelColor = newColor;
        }
    }

    if (!SetDIBits(hdc, hbmp, 0, bitmap.bmHeight, &pixels[0], &bitmapInfo, DIB_RGB_COLORS)) {
        WARNING_PRINTF("failed to set bitmap bits, SetDIBits() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    ::ReleaseDC(HWND_DESKTOP, hdc);
}

} // anonymous namespace
