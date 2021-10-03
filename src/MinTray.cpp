// Copyright 2020 Benbuck Nason

#include "DebugPrint.h"
#include "File.h"
#include "Hotkey.h"
#include "Resource.h"
#include "Settings.h"
#include "TrayIcon.h"
#include "TrayWindow.h"

#include <Windows.h>
// #include <oleacc.h>
#include <string>

static constexpr WCHAR APP_NAME[] = L"MinTray";
static constexpr WORD IDM_EXIT = 0x1003;
static constexpr WORD IDM_ABOUT = 0x1004;

enum class HotkeyID
{
    Minimize = 1,
    Restore
};

static LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void showContextMenu(HWND hwnd);
static std::wstring getResourceString(UINT id);
static inline void errorMessage(UINT id);

#if 0
VOID CALLBACK
winEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime)
{
    // DEBUG_PRINTF(".\n");

    switch (event) {
        case EVENT_SYSTEM_FOREGROUND:
        case EVENT_SYSTEM_CAPTURESTART:
        case EVENT_SYSTEM_CAPTUREEND:
        case EVENT_SYSTEM_DIALOGSTART:
        case EVENT_OBJECT_SHOW:
        case EVENT_OBJECT_HIDE:
        case EVENT_OBJECT_REORDER:
        case EVENT_OBJECT_FOCUS:
        case EVENT_OBJECT_STATECHANGE:
        case EVENT_OBJECT_LOCATIONCHANGE:
        case EVENT_OBJECT_NAMECHANGE:
        case EVENT_OBJECT_VALUECHANGE:
        case EVENT_OBJECT_PARENTCHANGE: {
            break;
        }

        case EVENT_OBJECT_CREATE:
        case EVENT_OBJECT_DESTROY:
        case EVENT_SYSTEM_MINIMIZESTART:
        case EVENT_SYSTEM_MINIMIZEEND: {
            IAccessible * pAcc = NULL;
            VARIANT varChild;
            HRESULT hr = AccessibleObjectFromEvent(hwnd, idObject, idChild, &pAcc, &varChild);
            if ((hr == S_OK) && (pAcc != NULL)) {
                IDispatch * dispatch = nullptr;
                pAcc->get_accParent(&dispatch);
                if (dispatch) {
                    dispatch->Release();
                } else {
                    BSTR bstrName;
                    pAcc->get_accName(varChild, &bstrName);
                    DEBUG_PRINTF("%S\n", bstrName);
                    SysFreeString(bstrName);
                }

                pAcc->Release();
            }
            break;
        }

        default: {
            DEBUG_PRINTF("event %X\n", event);
            break;
        }
    }
}
#endif

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    // unused
    (void)hPrevInstance;
    (void)pCmdLine;

    Settings settings_;

    // get settings from file
    const WCHAR settingsFileName[] = L"MinTray.json";
    const char * json = fileRead(settingsFileName);
    if (!json) {
        // no settings file in current directory, try in executable dir
        WCHAR fileName[MAX_PATH];
        std::wstring exePath = getExecutablePath();
        _snwprintf_s(fileName, MAX_PATH, L"%s\\%s", exePath.c_str(), settingsFileName);
        json = fileRead(fileName);
    }
    if (json) {
        settings_.parseJson(json);
        delete[] json;
    }

    // get settings from command line (can override file)
    settings_.parseCommandLine();

    HICON icon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MINTRAY));

    // create the window class
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = APP_NAME;
    wc.hIcon = icon;
    wc.hIconSm = icon;
    ATOM atom = RegisterClassEx(&wc);
    if (!atom) {
        errorMessage(IDS_ERROR_REGISTER_WINDOW_CLASS);
        return 0;
    }

    // create the window
    HWND hwnd = CreateWindow(
        APP_NAME, // class name
        APP_NAME, // window name
        WS_OVERLAPPEDWINDOW, // style
        0, // x
        0, // y
        0, // w
        0, // h
        NULL, // parent
        NULL, // menu
        hInstance, // instance
        NULL // application data
    );
    if (!hwnd) {
        errorMessage(IDS_ERROR_CREATE_WINDOW);
        return 0;
    }

    // we intentionally don't show the window
    // ShowWindow(hwnd, nCmdShow);
    (void)nCmdShow;

    // register a hotkey that will be used to minimize windows
    Hotkey hotkeyMinimize;
    if (!hotkeyMinimize.create((INT)HotkeyID::Minimize, hwnd, VK_DOWN, MOD_WIN | MOD_ALT | MOD_NOREPEAT)) {
        errorMessage(IDS_ERROR_REGISTER_HOTKEY);
        return 0;
    }

    // register a hotkey that will be used to restore windows
    Hotkey hotkeyRestore;
    if (!hotkeyRestore.create((INT)HotkeyID::Restore, hwnd, VK_UP, MOD_WIN | MOD_ALT | MOD_NOREPEAT)) {
        errorMessage(IDS_ERROR_REGISTER_HOTKEY);
        return 0;
    }

    // create a tray icon for the app
    TrayIcon trayIcon;
    if (!trayIcon.create(hwnd, WM_TRAYWINDOW, icon)) {
        errorMessage(IDS_ERROR_CREATE_TRAY_ICON);
        return 0;
    }

    // CoInitialize(NULL);
    // HWINEVENTHOOK hWinEventHook =
    //     SetWinEventHook(EVENT_MIN, EVENT_MAX, NULL, winEventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    // run the message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // UnhookWinEvent(hWinEventHook);
    // CoUninitialize();

    return 0;
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT taskbarCreatedMessage;

    switch (uMsg) {
        // command from context menu
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                // about dialog
                case IDM_ABOUT: {
                    std::wstring const & aboutTextStr = getResourceString(IDS_ABOUT_TEXT);
                    std::wstring const & aboutCaptionStr = getResourceString(IDS_ABOUT_CAPTION);
                    MessageBox(hwnd, aboutTextStr.c_str(), aboutCaptionStr.c_str(), MB_OK | MB_ICONINFORMATION);
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
            trayWindowRestoreAll();

            // exit
            PostQuitMessage(0);
            return 0;
        }

        // our hotkey was activated
        case WM_HOTKEY: {
            HotkeyID hkid = (HotkeyID)wParam;
            switch (hkid) {
                case HotkeyID::Minimize: {
                    // get the foreground window to minimize
                    HWND hwndFg = GetForegroundWindow();
                    if (hwndFg) {
                        // only minimize windows that have a minimize button
                        LONG windowStyle = GetWindowLong(hwndFg, GWL_STYLE);
                        if (windowStyle & WS_MINIMIZEBOX) {
#if !defined(NDEBUG)
                            WCHAR text[256];
                            GetWindowText(hwndFg, text, sizeof(text) / sizeof(text[0]));
                            DEBUG_PRINTF("window text '%ls'\n", text);

                            WCHAR className[256];
                            GetClassName(hwndFg, className, sizeof(className) / sizeof(className[0]));
                            DEBUG_PRINTF("window class name '%ls'\n", className);
#endif

                            trayWindowMinimize(hwndFg, hwnd);
                        }
                    }
                    break;
                }

                case HotkeyID::Restore: {
                    HWND hwndLast = trayWindowGetLast();
                    if (hwndLast) {
                        trayWindowRestore(hwndLast);
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
                // user selected and activated icon
                case NIN_SELECT: {
                    HWND hwndTray = trayWindowGetFromID((UINT)wParam);
                    trayWindowRestore(hwndTray);
                    break;
                }

                // user activated context menu
                case WM_CONTEXTMENU: {
                    HWND hwndTray = trayWindowGetFromID((UINT)wParam);
                    if (!hwndTray) {
                        showContextMenu(hwnd);
                    }
                    break;
                }

#if 0
                case WM_MOUSEMOVE: {
                    HWND hwndTray = trayWindowGetFromID((UINT)wParam);
                    trayWindowRefresh(hwndTray);
                    break;
                }
#endif
            }
            break;
        }

        default: {
            if (uMsg == taskbarCreatedMessage) {
                trayWindowAddAll();
            }
            break;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void showContextMenu(HWND hwnd)
{
    // create popup menu
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }

    // add menu entries
    AppendMenu(hMenu, MF_STRING | MF_DISABLED, 0, APP_NAME);
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_ABOUT, getResourceString(IDS_MENU_ABOUT).c_str());
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, getResourceString(IDS_MENU_EXIT).c_str());

    // activate our window
    SetForegroundWindow(hwnd);

    // get the current mouse position
    POINT point = { 0, 0 };
    GetCursorPos(&point);

    // show the popup menu
    if (!TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hwnd, NULL)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        DestroyMenu(hMenu);
        return;
    }

    // force a task switch to our app
    PostMessage(hwnd, WM_USER, 0, 0);

    DestroyMenu(hMenu);
}

std::wstring getResourceString(UINT id)
{
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    std::wstring str;
    str.resize(256);
    LoadString(hInstance, id, &str[0], (int)str.size());

    return str;
}

void errorMessage(UINT id)
{
    std::wstring const & err = getResourceString(id);
    MessageBox(NULL, err.c_str(), APP_NAME, MB_OK | MB_ICONERROR);
}
