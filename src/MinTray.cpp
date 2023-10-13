// Copyright 2020 Benbuck Nason

// MinTray
#include "CommandLine.h"
#include "DebugPrint.h"
#include "File.h"
#include "Hotkey.h"
#include "Resource.h"
#include "Settings.h"
#include "StringUtilities.h"
#include "TrayIcon.h"
#include "TrayWindow.h"
#include "WindowList.h"

// windows
#include <Psapi.h>
#include <Windows.h>

// standard library
#include <regex>
#include <set>

static constexpr CHAR APP_NAME[] = "MinTray";
static constexpr WORD IDM_EXIT = 0x1003;
static constexpr WORD IDM_ABOUT = 0x1004;

enum class HotkeyID
{
    Minimize = 1,
    Restore
};

static LRESULT wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void onNewWindow(HWND hwnd);
static void showContextMenu(HWND hwnd);
static std::string getResourceString(UINT id);
static inline void errorMessage(UINT id);

static Settings settings_;
static HWND hwnd_;

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    // unused
    (void)hPrevInstance;
    (void)pCmdLine;

    // get settings from file
    std::string fileName(std::string(APP_NAME) + ".json");
    if (settings_.readFromFile(fileName)) {
        DEBUG_PRINTF("read settings from %ws\n", fileName.c_str());
    } else {
        // no settings file in current directory, try in executable dir
        std::string exePath = getExecutablePath();
        fileName = exePath + "\\" + std::string(APP_NAME) + ".json";
        if (settings_.readFromFile(fileName)) {
            DEBUG_PRINTF("read settings from %ws\n", fileName.c_str());
        }
    }

    int argc;
    char ** argv;
    if (!CommandLine::getArgs(&argc, &argv)) {
        errorMessage(IDS_ERROR_COMMAND_LINE);
        return IDS_ERROR_COMMAND_LINE;
    }
    bool parsed = settings_.parseCommandLine(argc, argv);
    CommandLine::freeArgs(argc, argv);
    if (!parsed) {
        errorMessage(IDS_ERROR_COMMAND_LINE);
        return IDS_ERROR_COMMAND_LINE;
    }

    HICON icon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MINTRAY));

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
        errorMessage(IDS_ERROR_CREATE_WINDOW);
        return IDS_ERROR_CREATE_WINDOW;
    }

    hwnd_ = hwnd;

    // we intentionally don't show the window
    // ShowWindow(hwnd, nCmdShow);
    (void)nCmdShow;

    // register a hotkey that will be used to minimize windows
    Hotkey hotkeyMinimize;
    UINT vkMinimize = VK_DOWN;
    UINT modifiersMinimize = MOD_WIN | MOD_ALT;
    if (!Hotkey::parse(settings_.hotkeyMinimize_, vkMinimize, modifiersMinimize)) {
        errorMessage(IDS_ERROR_REGISTER_HOTKEY);
        return IDS_ERROR_REGISTER_HOTKEY;
    }
    if (vkMinimize && modifiersMinimize) {
        if (!hotkeyMinimize.create((INT)HotkeyID::Minimize, hwnd, vkMinimize, modifiersMinimize | MOD_NOREPEAT)) {
            errorMessage(IDS_ERROR_REGISTER_HOTKEY);
            return IDS_ERROR_REGISTER_HOTKEY;
        }
    }

    // register a hotkey that will be used to restore windows
    Hotkey hotkeyRestore;
    UINT vkRestore = VK_UP;
    UINT modifiersRestore = MOD_WIN | MOD_ALT;
    if (!Hotkey::parse(settings_.hotkeyRestore_, vkRestore, modifiersRestore)) {
        errorMessage(IDS_ERROR_REGISTER_HOTKEY);
        return IDS_ERROR_REGISTER_HOTKEY;
    }
    if (vkRestore && modifiersRestore) {
        if (!hotkeyRestore.create((INT)HotkeyID::Restore, hwnd, vkRestore, modifiersRestore | MOD_NOREPEAT)) {
            errorMessage(IDS_ERROR_REGISTER_HOTKEY);
            return IDS_ERROR_REGISTER_HOTKEY;
        }
    }

    // create a tray icon for the app
    TrayIcon trayIcon;
    if (settings_.trayIcon_) {
        if (!trayIcon.create(hwnd, WM_TRAYWINDOW, icon)) {
            errorMessage(IDS_ERROR_CREATE_TRAY_ICON);
            return IDS_ERROR_CREATE_TRAY_ICON;
        }
    }

    windowListStart(hwnd, settings_.pollInterval_, onNewWindow);

    // run the message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    windowListStop();

    if (!DestroyWindow(hwnd)) {
        DEBUG_PRINTF("failed to destroy window: %#x\n", hwnd);
    }

    return 0;
}

LRESULT wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT taskbarCreatedMessage;

    switch (uMsg) {
        // command from context menu
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                // about dialog
                case IDM_ABOUT: {
                    std::string const & aboutTextStr = getResourceString(IDS_ABOUT_TEXT);
                    std::string const & aboutCaptionStr = getResourceString(IDS_ABOUT_CAPTION);
                    MessageBoxA(hwnd, aboutTextStr.c_str(), aboutCaptionStr.c_str(), MB_OK | MB_ICONINFORMATION);
                    break;
                }

                // exit the app
                case IDM_EXIT: {
                    SendMessage(hwnd, WM_DESTROY, 0, 0);
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
                            WCHAR text[256];
                            GetWindowText(hwndFg, text, sizeof(text) / sizeof(text[0]));
                            DEBUG_PRINTF("window text '%ws'\n", text);

                            WCHAR className[256];
                            GetClassName(hwndFg, className, sizeof(className) / sizeof(className[0]));
                            DEBUG_PRINTF("window class name '%ws'\n", className);
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
                    showContextMenu(hwnd);
                    break;
                }

                // user selected and activated icon
                case NIN_SELECT: {
                    HWND hwndTray = TrayWindow::getFromID((UINT)wParam);
                    if (hwndTray) {
                        TrayWindow::restore(hwndTray);
                    }
                    break;
                }

                default: DEBUG_PRINTF("traywindow message %ld\n", lParam); break;
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

static std::set<HWND> autoTrayedWindows_;

void onNewWindow(HWND hwnd)
{
    DEBUG_PRINTF("new window: %#x\n", hwnd);

    if (autoTrayedWindows_.find(hwnd) != autoTrayedWindows_.end()) {
        DEBUG_PRINTF("\tignoring, previously auto-trayed\n");
        return;
    }

    CHAR executable[MAX_PATH] = { 0 };
    DWORD dwProcId = 0;
    if (!GetWindowThreadProcessId(hwnd, &dwProcId)) {
        DEBUG_PRINTF("GetWindowThreadProcessId() failed: %u\n", GetLastError());
    } else {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId);
        if (!hProc) {
            DEBUG_PRINTF("OpenProcess() failed: %u\n", GetLastError());
        } else {
            if (!GetModuleFileNameExA((HMODULE)hProc, nullptr, executable, MAX_PATH)) {
                DEBUG_PRINTF("GetModuleFileNameA() failed: %u\n", GetLastError());
            } else {
                DEBUG_PRINTF("\texecutable: %s\n", executable);
            }
        }
        CloseHandle(hProc);
    }

    CHAR windowText[128];
    if (!GetWindowTextA(hwnd, windowText, sizeof(windowText))) {
        // DEBUG_PRINTF("failed to get window text %#x\n", hwnd);
    } else {
        DEBUG_PRINTF("\ttitle: %s\n", windowText);
    }

    CHAR className[1024];
    if (!GetClassNameA(hwnd, className, sizeof(className))) {
        DEBUG_PRINTF("failed to get window class name %#x\n", hwnd);
    } else {
        DEBUG_PRINTF("\tclass: %s\n", className);
    }

    for (auto const & autoTray : settings_.autoTrays_) {
        bool executableMatch = false;
        if (autoTray.executable_.empty()) {
            executableMatch = true;
        } else {
            if (autoTray.executable_ == executable) {
                DEBUG_PRINTF("\texecutable %s match\n", autoTray.executable_.c_str());
                executableMatch = true;
            }
        }

        bool classMatch = false;
        if (autoTray.windowClass_.empty()) {
            classMatch = true;
        } else {
            if (autoTray.windowClass_ == className) {
                DEBUG_PRINTF("\twindow class %s match\n", autoTray.windowClass_.c_str());
                classMatch = true;
            }
        }

        bool titleMatch = false;
        if (autoTray.windowTitle_.empty()) {
            titleMatch = true;
        } else {
            std::regex re(autoTray.windowTitle_);
            if (std::regex_match(windowText, re)) {
                DEBUG_PRINTF("\twindow title %s match\n", autoTray.windowTitle_.c_str());
                titleMatch = true;
            }
        }

        if (executableMatch && classMatch && titleMatch) {
            DEBUG_PRINTF("\t--- minimizing ---\n");
            TrayWindow::minimize(hwnd, hwnd_);
            autoTrayedWindows_.insert(hwnd);
        }
    }
}

void showContextMenu(HWND hwnd)
{
    // create popup menu
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }

    // add menu entries
    if (!AppendMenuA(menu, MF_STRING | MF_DISABLED, 0, APP_NAME)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }
    if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }
    if (!AppendMenuA(menu, MF_STRING, IDM_ABOUT, getResourceString(IDS_MENU_ABOUT).c_str())) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }
    if (!AppendMenuA(menu, MF_STRING, IDM_EXIT, getResourceString(IDS_MENU_EXIT).c_str())) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }

    // activate our window
    if (!SetForegroundWindow(hwnd)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }

    // get the current mouse position
    POINT point = { 0, 0 };
    if (!GetCursorPos(&point)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }

    // show the popup menu
    if (!TrackPopupMenu(menu, 0, point.x, point.y, 0, hwnd, nullptr)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        if (!DestroyMenu(menu)) {
            DEBUG_PRINTF("failed to destroy menu: %#x\n", menu);
        }
        return;
    }

    // force a task switch to our app
    if (!PostMessage(hwnd, WM_USER, 0, 0)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }

    if (!DestroyMenu(menu)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }
}

std::string getResourceString(UINT id)
{
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);

    std::string str;
    str.resize(256);
    if (!LoadStringA(hInstance, id, &str[0], (int)str.size())) {
        DEBUG_PRINTF("failed to load resources string %u\n", id);
    }

    return str;
}

void errorMessage(UINT id)
{
    DWORD lastError = GetLastError();
    std::string const & err = getResourceString(id);
    DEBUG_PRINTF("error: %s: %u\n", err.c_str(), lastError);
    (void)lastError;
    if (!MessageBoxA(nullptr, err.c_str(), APP_NAME, MB_OK | MB_ICONERROR)) {
        DEBUG_PRINTF("failed to display error message %u\n", id);
    }
}
