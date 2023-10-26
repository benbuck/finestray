// Copyright 2020 Benbuck Nason

#include "SettingsDialog.h"

#include "DebugPrint.h"
#include "Hotkey.h"
#include "Resource.h"

#include <CommCtrl.h>
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

            HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);
            HWND autoTrayList = GetDlgItem(hwndDlg, IDC_AUTO_TRAY_LIST);
            CHAR szText[256];

            LVCOLUMNA lvc;
            memset(&lvc, 0, sizeof(lvc));
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.iSubItem = 0;
            lvc.pszText = szText;
            lvc.cx = 100;
            lvc.fmt = LVCFMT_LEFT;
            LoadStringA(hInstance, IDS_COLUMN_EXECUTABLE, szText, sizeof(szText) / sizeof(szText[0]));
            if (SendMessageA(autoTrayList, LVM_INSERTCOLUMNA, lvc.iSubItem, (LPARAM)&lvc) == -1) {
                __debugbreak();
                // return FALSE;
            }

            lvc.iSubItem = 1;
            LoadStringA(hInstance, IDS_COLUMN_WINDOW_CLASS, szText, sizeof(szText) / sizeof(szText[0]));
            if (SendMessageA(autoTrayList, LVM_INSERTCOLUMNA, lvc.iSubItem, (LPARAM)&lvc) == -1) {
                __debugbreak();
                // return FALSE;
            }

            lvc.iSubItem = 2;
            LoadStringA(hInstance, IDS_COLUMN_WINDOW_TITLE, szText, sizeof(szText) / sizeof(szText[0]));
            if (SendMessageA(autoTrayList, LVM_INSERTCOLUMNA, lvc.iSubItem, (LPARAM)&lvc) == -1) {
                __debugbreak();
                // return FALSE;
            }

            LVITEMA lvI;
            memset(&lvI, 0, sizeof(lvI));
            lvI.mask = LVIF_TEXT;
            lvI.pszText = LPSTR_TEXTCALLBACKA; // sends an LVN_GETDISPINFO message to get item contents

            for (size_t a = 0; a < settings_.autoTrays_.size(); ++a) {
                lvI.iItem = (int)a;
                if (SendMessageA(autoTrayList, LVM_INSERTITEMA, 0, (LPARAM)&lvI) == -1) {
                    DEBUG_PRINTF("err %#x\n", GetLastError());
                    __debugbreak();
                    // return FALSE;
                }
            }

            break;
        }

        case WM_NOTIFY: {
            switch (((LPNMHDR)lParam)->code) {
                case LVN_GETDISPINFOA: {
                    DEBUG_PRINTF("LVN_GETDISPINFO\n");
                    NMLVDISPINFOA * plvdi = (NMLVDISPINFOA *)lParam;
                    switch (plvdi->item.iSubItem) {
                        case 0: {
                            plvdi->item.pszText = const_cast<CHAR *>(
                                settings_.autoTrays_[plvdi->item.iItem].executable_.c_str());
                            break;
                        }
                        case 1: {
                            plvdi->item.pszText = const_cast<CHAR *>(
                                settings_.autoTrays_[plvdi->item.iItem].windowClass_.c_str());
                            break;
                        }
                        case 2: {
                            plvdi->item.pszText = const_cast<CHAR *>(
                                settings_.autoTrays_[plvdi->item.iItem].windowTitle_.c_str());
                            break;
                        }
                        default: {
                            __debugbreak();
                            break;
                        }
                    }
                    break;
                }
                default: {
                    DEBUG_PRINTF("WM_NOTIFY default %x\n", wParam);
                    break;
                }
            }
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
