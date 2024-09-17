// Copyright 2020 Benbuck Nason

#include "SettingsDialog.h"

// MinTray
#include "DebugPrint.h"
#include "Hotkey.h"
#include "Resource.h"
#include "StringUtility.h"

// Windows
#include <CommCtrl.h>
#include <Windows.h>

namespace SettingsDialog
{

static INT_PTR dialogFunc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
static int listViewCompare(LPARAM, LPARAM, LPARAM);
static void initListView(HWND hwndDlg);

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

    switch (message) {
        case WM_INITDIALOG: {
            SetDlgItemTextA(hwndDlg, IDC_HOTKEY_MINIMIZE, settings_.hotkeyMinimize_.c_str());
            SetDlgItemTextA(hwndDlg, IDC_HOTKEY_RESTORE, settings_.hotkeyRestore_.c_str());
            SetDlgItemTextA(hwndDlg, IDC_MODIFIER_OVERRIDE, settings_.modifiersOverride_.c_str());
            SetDlgItemTextA(hwndDlg, IDC_POLL_INTERVAL, std::to_string(settings_.pollInterval_).c_str());

            initListView(hwndDlg);

            break;
        }

        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            DEBUG_PRINTF("nmhdr hwnd %#x, id %#x, code %d\n", nmhdr->hwndFrom, nmhdr->idFrom, (int)nmhdr->code);
            switch (nmhdr->code) {
                case LVN_GETDISPINFOW: {
                    DEBUG_PRINTF("LVN_GETDISPINFOW\n");
                    break;
                }
                case LVN_GETDISPINFOA: {
                    DEBUG_PRINTF("LVN_GETDISPINFOA\n");
                    NMLVDISPINFOA * displayInfo = (NMLVDISPINFOA *)lParam;
                    const char * str = nullptr;
                    switch (displayInfo->item.iSubItem) {
                        case 0: {
                            str = settings_.autoTrays_[displayInfo->item.iItem].executable_.c_str();
                            break;
                        }
                        case 1: {
                            str = settings_.autoTrays_[displayInfo->item.iItem].windowClass_.c_str();
                            break;
                        }
                        case 2: {
                            str = settings_.autoTrays_[displayInfo->item.iItem].windowTitle_.c_str();
                            break;
                        }
                        default: {
                            __debugbreak();
                            break;
                        }
                    }
                    if (!str || !strlen(str)) {
                        static char emptyStr[256];
                        if (!strlen(emptyStr)) {
                            HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);
                            LoadStringA(hInstance, IDS_ITEM_EMPTY, emptyStr, sizeof(emptyStr) / sizeof(emptyStr[0]));
                        }
                        str = emptyStr;
                    }
                    displayInfo->item.pszText = const_cast<CHAR *>(str);
                    break;
                }

                case LVN_COLUMNCLICK: {
                    NMLISTVIEW * pListView = (NMLISTVIEW *)lParam;
                    HWND autoTrayList = GetDlgItem(hwndDlg, IDC_AUTO_TRAY_LIST);
                    if (SendMessageA(autoTrayList, LVM_SORTITEMS, (WPARAM)pListView->iSubItem, (LPARAM)listViewCompare)) {
                        DEBUG_PRINTF("err %s\n", StringUtility::lastErrorString().c_str());
                        __debugbreak();
                        break;
                    }
                }

                case LVN_DELETEALLITEMS: {
                    DEBUG_PRINTF("LVN_DELETEALLITEMS\n");
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

int listViewCompare(LPARAM, LPARAM, LPARAM)
{
    DEBUG_PRINTF("listViewCompare\n");
    return -1;
}

void initListView(HWND hwndDlg)
{
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);
    HWND autoTrayList = GetDlgItem(hwndDlg, IDC_AUTO_TRAY_LIST);
    CHAR szText[256];

    LVCOLUMNA listViewColumn;
    memset(&listViewColumn, 0, sizeof(listViewColumn));
    listViewColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    listViewColumn.fmt = LVCFMT_LEFT;
    listViewColumn.cx = 100;
    listViewColumn.pszText = szText;

    listViewColumn.iSubItem = 0;
    LoadStringA(hInstance, IDS_COLUMN_EXECUTABLE, szText, sizeof(szText) / sizeof(szText[0]));
    if (SendMessageA(autoTrayList, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) == -1) {
        __debugbreak();
        // return FALSE;
    }
    if (SendMessageA(autoTrayList, LVM_SETCOLUMNWIDTH, listViewColumn.iSubItem, (LPARAM)LVSCW_AUTOSIZE_USEHEADER) == -1) {
        __debugbreak();
        // return FALSE;
    }

    ++listViewColumn.iSubItem = 1;
    LoadStringA(hInstance, IDS_COLUMN_WINDOW_CLASS, szText, sizeof(szText) / sizeof(szText[0]));
    if (SendMessageA(autoTrayList, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) == -1) {
        __debugbreak();
        // return FALSE;
    }
    if (SendMessageA(autoTrayList, LVM_SETCOLUMNWIDTH, listViewColumn.iSubItem, (LPARAM)LVSCW_AUTOSIZE_USEHEADER) == -1) {
        __debugbreak();
        // return FALSE;
    }

    listViewColumn.iSubItem = 2;
    LoadStringA(hInstance, IDS_COLUMN_WINDOW_TITLE, szText, sizeof(szText) / sizeof(szText[0]));
    if (SendMessageA(autoTrayList, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) == -1) {
        __debugbreak();
        // return FALSE;
    }
    if (SendMessageA(autoTrayList, LVM_SETCOLUMNWIDTH, listViewColumn.iSubItem, (LPARAM)LVSCW_AUTOSIZE_USEHEADER) == -1) {
        __debugbreak();
        // return FALSE;
    }

    if (SendMessageA(autoTrayList, LVM_SETITEMCOUNT, (WPARAM)settings_.autoTrays_.size(), 0) == -1) {
        __debugbreak();
        // return FALSE;
    }

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_TEXT;
    listViewItem.pszText = LPSTR_TEXTCALLBACKA; // sends an LVN_GETDISPINFO message to get item contents

    for (size_t a = 0; a < settings_.autoTrays_.size(); ++a) {
        listViewItem.iItem = (int)a;
        if (SendMessageA(autoTrayList, LVM_INSERTITEMA, 0, (LPARAM)&listViewItem) == -1) {
            DEBUG_PRINTF("err %#x\n", StringUtility::lastErrorString().c_str());
            __debugbreak();
            // return FALSE;
        }
    }
}

} // namespace SettingsDialog
