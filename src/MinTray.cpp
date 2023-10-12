// Copyright 2020 Benbuck Nason

#include "DebugPrint.h"
#include "File.h"
#include "Hotkey.h"
#include "Resource.h"
#include "Settings.h"
#include "TrayIcon.h"
#include "TrayWindow.h"
#include "WindowList.h"

// windows
#include <Windows.h>
// #include <oleacc.h>

// standard library
#include <regex>

static constexpr WCHAR APP_NAME[] = L"MinTray";
static constexpr WORD IDM_EXIT = 0x1003;
static constexpr WORD IDM_ABOUT = 0x1004;

enum class HotkeyID
{
    Minimize = 1,
    Restore
};

static LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void onNewWindow(HWND hwnd);
static void showContextMenu(HWND hwnd);
static std::wstring getResourceString(UINT id);
static inline void errorMessage(UINT id);

static Settings settings_;
static HWND hwnd_;

#if 0

#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <Wbemidl.h>
#include <atlcomcli.h>
#include <comdef.h>

#pragma comment(lib, "wbemuuid.lib")
namespace CreationEvent
{
void registerCreationCallback(void (*callback)(void));
} // namespace CreationEvent

class EventSink : public IWbemObjectSink
{
    friend void CreationEvent::registerCreationCallback(void (*callback)(void));

    CComPtr<IWbemServices> pSvc;
    CComPtr<IWbemObjectSink> pStubSink;
    LONG m_IRef;
    void (*m_callback)(void);

public:
    EventSink(void (*callback)(void)) : m_IRef(0), m_callback(callback) {}
    ~EventSink() {}

    virtual ULONG STDMETHODCALLTYPE AddRef() { return InterlockedIncrement(&m_IRef); }

    virtual ULONG STDMETHODCALLTYPE Release()
    {
        LONG IRef = InterlockedDecrement(&m_IRef);
        if (IRef == 0) delete this;
        return IRef;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppv)
    {
        if (riid == IID_IUnknown || riid == IID_IWbemObjectSink) {
            *ppv = (IWbemObjectSink *)this;
            AddRef();
            return WBEM_S_NO_ERROR;
        } else
            return E_NOINTERFACE;
    }

    virtual HRESULT STDMETHODCALLTYPE Indicate(LONG lObjectCount, IWbemClassObject __RPC_FAR * __RPC_FAR * apObjArray)
    {
        for (int i = 0; i < lObjectCount; ++i) {
            (void)lObjectCount;
            (void)apObjArray;

            m_callback();

            HRESULT hr;

            IWbemClassObject * obj = apObjArray[i];
            BSTR objText = NULL;
            hr = obj->GetObjectText(0, &objText);
            if (SUCCEEDED(hr) && objText) {
                DEBUG_PRINTF("objtext:\n-----\n%S\n-----\n", objText);
                SysFreeString(objText);
            }

            SAFEARRAY * names = NULL;
            hr = obj->GetNames(NULL, WBEM_FLAG_ALWAYS, NULL, &names);
            if (FAILED(hr)) __debugbreak();
            // UINT dim = SafeArrayGetDim(names);
            LONG lstart, lend;
            hr = SafeArrayGetLBound(names, 1, &lstart);
            if (FAILED(hr)) __debugbreak();
            hr = SafeArrayGetUBound(names, 1, &lend);
            if (FAILED(hr)) __debugbreak();
            BSTR * pbstr;
            hr = SafeArrayAccessData(names, (void HUGEP **)&pbstr);
            if (FAILED(hr)) __debugbreak();
            for (LONG idx = lstart; idx < lend; ++idx) {
                VARIANT v;

                // VariantInit(&v);
                // hr = SafeArrayGetElement(names, &idx, &v);
                // if (FAILED(hr)) __debugbreak();
                // if (V_VT(&v) == VT_BSTR) {
                //     DEBUG_PRINTF("the class name is %S\n.", V_BSTR(&v));
                // } else {
                //     DEBUG_PRINTF("got %d\n", V_VT(&v));
                // }

                CIMTYPE pType;
                hr = obj->Get(pbstr[idx], 0, &v, &pType, 0);
                if (v.vt == VT_NULL) {
                    continue;
                }
                if (pType == CIM_STRING && pType != CIM_EMPTY && pType != CIM_ILLEGAL) {
                    DEBUG_PRINTF("os Name : %d %S\n", idx, v.bstrVal);
                }

                VariantClear(&v);
            }
            hr = SafeArrayUnaccessData(names);
            if (FAILED(hr)) __debugbreak();

            SafeArrayDestroy(names);

            hr = obj->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY);
            if (FAILED(hr)) __debugbreak();
            while (true) {
                BSTR name;
                VARIANT val;
                CIMTYPE type;
                long flavor;
                hr = obj->Next(0, &name, &val, &type, &flavor);
                if (FAILED(hr)) __debugbreak();
                if (hr == WBEM_S_NO_MORE_DATA) {
                    break;
                }
                SysFreeString(name);
                VariantClear(&val);
            }
            hr = obj->EndEnumeration();
            if (FAILED(hr)) __debugbreak();

            VARIANT v;
            // hr = obj->Get(L"Name", 0, &v, 0, 0);
            hr = obj->Get(L"TargetInstance", 0, &v, 0, 0);
            // hr = obj->Get(L"this.TargetInstance.Name", 0, &v, 0, 0);
            // hr = obj->Get(L"TargetInstance.Name", 0, &v, 0, 0);
            // hr = obj->Get(L"TargetInstance::Name", 0, &v, 0, 0);
            // hr = obj->Get(L"TargetInstance\\Name", 0, &v, 0, 0);
            // hr = obj->Get(L"\\TargetInstance\\Name", 0, &v, 0, 0);
            // hr = obj->Get(L"TargetInstance/Name", 0, &v, 0, 0);
            // hr = obj->Get(L"/TargetInstance/Name", 0, &v, 0, 0);
            if (SUCCEEDED(hr)) {
                if (V_VT(&v) == VT_BSTR)
                    DEBUG_PRINTF("the class name is %S\n.", V_BSTR(&v));
                else
                    DEBUG_PRINTF("got %d\n", V_VT(&v));
            } else {
                DEBUG_PRINTF("error in getting specified object\n");
            }
            VariantClear(&v);
        }
        /* Unregister event sink */
        // pSvc->CancelAsyncCall(pStubSink);
        return WBEM_S_NO_ERROR;
    }
    virtual HRESULT STDMETHODCALLTYPE
    SetStatus(LONG /*IFlags*/, HRESULT /*hResult*/, BSTR /*strParam*/, IWbemClassObject __RPC_FAR * /*pObjParam*/)
    {
        return WBEM_S_NO_ERROR;
    }
};

void CreationEvent::registerCreationCallback(void (*callback)(void))
{
    CComPtr<IWbemLocator> pLoc;
    CoInitializeEx(0, COINIT_MULTITHREADED);
    // CoInitialize(NULL);

    HRESULT hres = CoInitializeSecurity(
        NULL,
        -1, // COM authentication
        NULL, // Authentication services
        NULL, // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT, // Default authentication
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
        NULL, // Authentication info
        EOAC_NONE, // Additional capabilities
        NULL // Reserved
    );
    if (FAILED(hres)) __debugbreak();

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
    if (FAILED(hres)) __debugbreak();

    CComPtr<EventSink> pSink(new EventSink(callback));

    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSink->pSvc);
    if (FAILED(hres)) __debugbreak();

    hres = CoSetProxyBlanket(
        pSink->pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE);
    if (FAILED(hres)) __debugbreak();

    CComPtr<IUnsecuredApartment> pUnsecApp;
    hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL, CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment, (void **)&pUnsecApp);
    CComPtr<IUnknown> pStubUnk;
    pUnsecApp->CreateObjectStub(pSink, &pStubUnk);
    pStubUnk->QueryInterface(IID_IWbemObjectSink, (void **)&pSink->pStubSink);

    char const query[] = "SELECT * FROM __InstanceCreationEvent WITHIN 1 WHERE TargetInstance ISA 'Win32_Process'";
    hres = pSink->pSvc->ExecNotificationQueryAsync(_bstr_t("WQL"), _bstr_t(query), WBEM_FLAG_SEND_STATUS, NULL, pSink->pStubSink);
    if (FAILED(hres)) __debugbreak();

    // FIX
    // // Cleanup
    // // ========
    //
    // hres = pSvc->CancelAsyncCall(pStubSink);
    // pSvc->Release();
    // pLoc->Release();
    // pEnumerator->Release();
    // CoUninitialize();
}

void k() { cout << "sadfdsa " << endl; }

#endif

#if 0
VOID CALLBACK
winEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime)
{
    // unused
    (void)hWinEventHook;
    (void)idEventThread;
    (void)dwmsEventTime;

    // DEBUG_PRINTF(".\n");

    if (hwnd == hwnd_) {
        return;
    }

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
        case EVENT_OBJECT_NAMECHANGE:
        case EVENT_OBJECT_VALUECHANGE:
        case EVENT_OBJECT_PARENTCHANGE:
        case EVENT_OBJECT_CLOAKED:
        case EVENT_OBJECT_UNCLOAKED: {
            break;
        }

        case EVENT_OBJECT_LOCATIONCHANGE: {
            WINDOWPLACEMENT wp;
            memset(&wp, 0, sizeof(wp));
            wp.length = sizeof(WINDOWPLACEMENT);
            if (GetWindowPlacement(hwnd, &wp)) {
                if (SW_SHOWMAXIMIZED == wp.showCmd) {
                    DEBUG_PRINTF("window is maximized.\n");
                }
            }

            break;
        }

        case EVENT_OBJECT_CREATE:
        case EVENT_OBJECT_DESTROY:
        case EVENT_SYSTEM_MINIMIZESTART:
        case EVENT_SYSTEM_MINIMIZEEND: {
            char title[1024];
            GetWindowTextA(hwnd, title, sizeof(title));
            DEBUG_PRINTF("event %#X hwnd %#X (%s) ido %#X idc %#X\n", event, hwnd, title, idObject, idChild);

            IAccessible * pAcc = NULL;
            VARIANT varChild;
            HRESULT hr = AccessibleObjectFromEvent(hwnd, idObject, idChild, &pAcc, &varChild);
            // if (FAILED(hr)) __debugbreak();
            if ((hr == S_OK) && (pAcc != NULL)) {
                IDispatch * dispatch = nullptr;
                hr = pAcc->get_accParent(&dispatch);
                if (FAILED(hr)) __debugbreak();
                if (dispatch) {
                    dispatch->Release();
                } else {
                    BSTR bstrName;
                    hr = pAcc->get_accName(varChild, &bstrName);
                    if (FAILED(hr)) __debugbreak();
                    if ((hr == S_OK) && bstrName) {
                        DEBUG_PRINTF("str %S\n", bstrName);
                        SysFreeString(bstrName);
                    }
                }

                pAcc->Release();
                VariantClear(&varChild);
            }
            break;
        }

        default: {
            char title[1024];
            GetWindowTextA(hwnd, title, sizeof(title));
            DEBUG_PRINTF("event %#X hwnd %#X (%s) ido %#X idc %#X\n", event, hwnd, title, idObject, idChild);
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

    // get settings from file
    std::wstring fileName(std::wstring(APP_NAME) + L".json");
    if (settings_.readFromFile(fileName)) {
        DEBUG_PRINTF("read setttings from %ls\n", fileName.c_str());
    } else {
        // no settings file in current directory, try in executable dir
        std::wstring exePath = getExecutablePath();
        fileName = exePath + L"\\" + std::wstring(APP_NAME) + L".json";
        if (settings_.readFromFile(fileName)) {
            DEBUG_PRINTF("read setttings from %ls\n", fileName.c_str());
        }
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
        return 0;
    }
    if (vkMinimize && modifiersMinimize) {
        if (!hotkeyMinimize.create((INT)HotkeyID::Minimize, hwnd, vkMinimize, modifiersMinimize | MOD_NOREPEAT)) {
            errorMessage(IDS_ERROR_REGISTER_HOTKEY);
            return 0;
        }
    }

    // register a hotkey that will be used to restore windows
    Hotkey hotkeyRestore;
    UINT vkRestore = VK_UP;
    UINT modifiersRestore = MOD_WIN | MOD_ALT;
    if (!Hotkey::parse(settings_.hotkeyRestore_, vkRestore, modifiersRestore)) {
        errorMessage(IDS_ERROR_REGISTER_HOTKEY);
        return 0;
    }
    if (vkRestore && modifiersRestore) {
        if (!hotkeyRestore.create((INT)HotkeyID::Restore, hwnd, vkRestore, modifiersRestore | MOD_NOREPEAT)) {
            errorMessage(IDS_ERROR_REGISTER_HOTKEY);
            return 0;
        }
    }

    // create a tray icon for the app
    TrayIcon trayIcon;
    if (settings_.trayIcon_) {
        if (!trayIcon.create(hwnd, WM_TRAYWINDOW, icon)) {
            errorMessage(IDS_ERROR_CREATE_TRAY_ICON);
            return 0;
        }
    }

#if 0
    //CreationEvent::registerCreationCallback(k);
    //cin.get();

    CoInitialize(NULL);
    //CoInitializeEx(0, COINIT_MULTITHREADED);
    DWORD pid = 0; // GetCurrentProcessId();
    DWORD tid = 0; // GetCurrentThreadId();
    hwnd_ = hwnd;
    HWINEVENTHOOK hWinEventHook = SetWinEventHook(
        EVENT_OBJECT_LOCATIONCHANGE,
        EVENT_OBJECT_LOCATIONCHANGE,
        NULL,
        winEventProc,
        pid,
        tid,
        WINEVENT_OUTOFCONTEXT);
    if (!hWinEventHook) __debugbreak();
#endif

    windowListStart(hwnd, settings_.enumWindowsIntervalMs_, onNewWindow);

    // run the message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    windowListStop();

#if 0
    UnhookWinEvent(hWinEventHook);
    CoUninitialize();
#endif

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
                    DEBUG_PRINTF("hotkey restore\n");
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
                // user activated context menu
                case WM_CONTEXTMENU: {
                    showContextMenu(hwnd);
                    break;
                }

                // user selected and activated icon
                case NIN_SELECT: {
                    HWND hwndTray = trayWindowGetFromID((UINT)wParam);
                    if (hwndTray) {
                        trayWindowRestore(hwndTray);
                    }
                    break;
                }

                case WM_MOUSEMOVE: {
                    HWND hwndTray = trayWindowGetFromID((UINT)wParam);
                    if (hwndTray) {
                        trayWindowRefresh(hwndTray);
                    }
                    break;
                }

                default: DEBUG_PRINTF("traywindow message %ld\n", lParam); break;
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

void onNewWindow(HWND hwnd)
{
    DEBUG_PRINTF("new window: %#x\n", hwnd);

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
        bool classMatch = false;
        if (autoTray.className_.empty()) {
            classMatch = true;
        } else {
            if (autoTray.className_ == className) {
                DEBUG_PRINTF("\tclassname %s match\n", autoTray.className_.c_str());
                classMatch = true;
            }
        }

        bool titleMatch = false;
        if (autoTray.titleRegex_.empty()) {
            titleMatch = true;
        } else {
            std::regex re(autoTray.titleRegex_);
            if (std::regex_match(windowText, re)) {
                DEBUG_PRINTF("\ttitle regex %s match\n", autoTray.titleRegex_.c_str());
                titleMatch = true;
            }
        }

        if (classMatch && titleMatch) {
            DEBUG_PRINTF("\t--- minimizing ---\n");
            trayWindowMinimize(hwnd, hwnd_);
        }
    }
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
    if (!AppendMenu(hMenu, MF_STRING | MF_DISABLED, 0, APP_NAME)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }
    if (!AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }
    if (!AppendMenu(hMenu, MF_STRING, IDM_ABOUT, getResourceString(IDS_MENU_ABOUT).c_str())) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }
    if (!AppendMenu(hMenu, MF_STRING, IDM_EXIT, getResourceString(IDS_MENU_EXIT).c_str())) {
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
    if (!TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hwnd, NULL)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        DestroyMenu(hMenu);
        return;
    }

    // force a task switch to our app
    if (!PostMessage(hwnd, WM_USER, 0, 0)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }

    if (!DestroyMenu(hMenu)) {
        errorMessage(IDS_ERROR_CREATE_POPUP_MENU);
        return;
    }
}

std::wstring getResourceString(UINT id)
{
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    std::wstring str;
    str.resize(256);
    if (!LoadString(hInstance, id, &str[0], (int)str.size())) {
        DEBUG_PRINTF("failed to load resources string %u\n", id);
    }

    return str;
}

void errorMessage(UINT id)
{
    DWORD lastError = GetLastError();
    std::wstring const & err = getResourceString(id);
    DEBUG_PRINTF("error: %ls: %u\n", err.c_str(), lastError);
    if (!MessageBox(NULL, err.c_str(), APP_NAME, MB_OK | MB_ICONERROR)) {
        DEBUG_PRINTF("failed to display error message %u\n", id);
    }
}
