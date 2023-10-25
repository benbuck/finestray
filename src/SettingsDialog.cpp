// Copyright 2020 Benbuck Nason

#include "SettingsDialog.h"

#include "DebugPrint.h"
#include "Hotkey.h"
#include "Resource.h"

#include <Windows.h>

namespace SettingsDialog
{

static INT_PTR dialogFunc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

static HWND hwnd_;
static Settings settings_;
static CompletionCallback completionCallback_;

HWND create(HWND hwnd, const Settings & settings, CompletionCallback completionCallback)
{
    hwnd_ = hwnd;
    settings_ = settings;
    completionCallback_ = completionCallback;

    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);
    HWND dlg = CreateDialogA(hInstance, MAKEINTRESOURCEA(IDD_DIALOG_SETTINGS), hwnd_, dialogFunc);
    ShowWindow(dlg, SW_SHOW);

    return dlg;
}

bool validateControl(HWND hwnd, WORD control)
{
    DEBUG_PRINTF("changed %#x\n", control);
    CHAR dlgItemText[256];
    if (!GetDlgItemTextA(hwnd, control, dlgItemText, sizeof(dlgItemText))) {
        *dlgItemText = 0;
    }
    UINT key = 0;
    UINT modifiers = 0;
    if (!Hotkey::parse(dlgItemText, key, modifiers)) {
        DEBUG_PRINTF("invalid hotkey\n");
        return false;
    }
    DEBUG_PRINTF("hotkey %s\n", dlgItemText);
    return true;
}

INT_PTR dialogFunc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DEBUG_PRINTF("wnd %#x, message %#x, wparam %#x, lparam %#x\n", hwndDlg, message, wParam, lParam);
    (void)lParam;

    switch (message) {
        case WM_INITDIALOG: {
            SetDlgItemTextA(hwndDlg, IDC_HOTKEY_MINIMIZE, settings_.hotkeyMinimize_.c_str());
            SetDlgItemTextA(hwndDlg, IDC_HOTKEY_RESTORE, settings_.hotkeyRestore_.c_str());
            SetDlgItemTextA(hwndDlg, IDC_MODIFIER_OVERRIDE, settings_.modifiersOverride_.c_str());
            SetDlgItemTextA(hwndDlg, IDC_POLL_INTERVAL, std::to_string(settings_.pollInterval_).c_str());
            break;
        }

        case WM_COMMAND: {
            switch (HIWORD(wParam)) {
                case EN_CHANGE: {
                    bool valid = validateControl(hwndDlg, LOWORD(wParam));
                    HWND hwndOk = GetDlgItem(hwndDlg, IDOK);
                    EnableWindow(hwndOk, valid);
                    break;
                }

                default: {
                    DEBUG_PRINTF("HIWORD %#x\n", HIWORD(wParam));
                    switch (LOWORD(wParam)) {
                        case IDOK: {
                            CHAR dlgItemText[256];
                            if (!GetDlgItemTextA(hwndDlg, IDC_HOTKEY_MINIMIZE, dlgItemText, sizeof(dlgItemText))) {
                                *dlgItemText = 0;
                            }
                            DEBUG_PRINTF("got dialog hotkey minimize %s\n", dlgItemText);
                            EndDialog(hwndDlg, wParam);
                            if (completionCallback_) {
                                completionCallback_(true, settings_);
                            }
                            return true;
                        }

                        case IDCANCEL: {
                            EndDialog(hwndDlg, wParam);
                            if (completionCallback_) {
                                completionCallback_(false, settings_);
                            }
                            return TRUE;
                        }
                    }
                }
            }
            break;
        }

        default: {
            DEBUG_PRINTF("message %#x, param %#x\n", message, wParam);
            break;
        }
    }

    return FALSE;
}

} // namespace SettingsDialog
