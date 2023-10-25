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
                    break;
                }

                default: {
                    DEBUG_PRINTF("HIWORD %#x\n", HIWORD(wParam));
                    switch (LOWORD(wParam)) {
                        case IDOK: {
                            CHAR dlgItemText[256];
                            if (GetDlgItemTextA(hwndDlg, IDC_HOTKEY_MINIMIZE, dlgItemText, sizeof(dlgItemText))) {
                                settings_.hotkeyMinimize_ = dlgItemText;
                            }
                            if (GetDlgItemTextA(hwndDlg, IDC_HOTKEY_RESTORE, dlgItemText, sizeof(dlgItemText))) {
                                settings_.hotkeyRestore_ = dlgItemText;
                            }
                            if (GetDlgItemTextA(hwndDlg, IDC_MODIFIER_OVERRIDE, dlgItemText, sizeof(dlgItemText))) {
                                settings_.modifiersOverride_ = dlgItemText;
                            }
                            if (GetDlgItemTextA(hwndDlg, IDC_POLL_INTERVAL, dlgItemText, sizeof(dlgItemText))) {
                                settings_.pollInterval_ = std::stoul(dlgItemText);
                            }

                            EndDialog(hwndDlg, wParam);

                            if (completionCallback_) {
                                completionCallback_(true, settings_);
                            }

                            return TRUE;
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
