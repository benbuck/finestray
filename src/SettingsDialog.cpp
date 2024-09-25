// Copyright 2020 Benbuck Nason

#include "SettingsDialog.h"

// App
#include "DebugPrint.h"
#include "Hotkey.h"
#include "MinTray.h"
#include "Resource.h"
#include "StringUtility.h"

// Windows
#include <CommCtrl.h>
#include <Windows.h>

// #define SORT_ENABLED

namespace SettingsDialog
{

enum class AutoTrayListViewColumn
{
    Executable,
    WindowClass,
    WindowTitle
};

static INT_PTR settingsDialogFunc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void autoTrayListViewInit(HWND hwndDlg);
static std::vector<Settings::AutoTray> autoTrayListViewGetItems(HWND hwndDlg);
static void autoTrayListViewNotify(HWND hwndDlg, LPNMHDR nmhdr);
#ifdef SORT_ENABLED
static int autoTrayListViewCompare(LPARAM, LPARAM, LPARAM);
#endif
static void autoTrayListViewItemAdd(HWND hwndDlg);
static void autoTrayListViewItemUpdate(HWND hwndDlg, int item);
static void autoTrayListViewItemDelete(HWND hwndDlg, int item);
static void autoTrayListViewItemEdit(HWND hwndDlg, int item);
static void autoTrayListViewUpdateButtons(HWND hwndDlg);
static void autoTrayListViewUpdateSelected(HWND hwndDlg);

static Settings settings_;
static CompletionCallback completionCallback_;
static HWND autoTrayListView_;
static int autoTrayListViewActiveItem_;
#ifdef SORT_ENABLED
static bool autoTrayListViewSortAscending_;
static int autoTrayListViewSortColumn_;
#endif

HWND create(HWND hwnd, const Settings & settings, CompletionCallback completionCallback)
{
    settings_ = settings;
    completionCallback_ = completionCallback;

    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);
    HWND dlg = CreateDialogA(hInstance, MAKEINTRESOURCEA(IDD_DIALOG_SETTINGS), hwnd, settingsDialogFunc);
    ShowWindow(dlg, SW_SHOW);

    return dlg;
}

INT_PTR settingsDialogFunc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    // DEBUG_PRINTF("wnd %#x, message %#x, wparam %#x, lparam %#x\n", hwndDlg, message, wParam, lParam);

    switch (message) {
        case WM_INITDIALOG: {
            SetDlgItemTextA(hwndDlg, IDC_HOTKEY_MINIMIZE, settings_.hotkeyMinimize_.c_str());
            SetDlgItemTextA(hwndDlg, IDC_HOTKEY_RESTORE, settings_.hotkeyRestore_.c_str());
            SetDlgItemTextA(hwndDlg, IDC_MODIFIER_OVERRIDE, settings_.modifiersOverride_.c_str());
            SetDlgItemTextA(hwndDlg, IDC_POLL_INTERVAL, std::to_string(settings_.pollInterval_).c_str());

            autoTrayListViewInit(hwndDlg);

            break;
        }

        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            // DEBUG_PRINTF("nmhdr hwnd %#x, id %#x, code %d\n", nmhdr->hwndFrom, nmhdr->idFrom, (int)nmhdr->code);
            if (nmhdr->hwndFrom == GetDlgItem(hwndDlg, IDC_AUTO_TRAY_LIST)) {
                autoTrayListViewNotify(hwndDlg, nmhdr);
            }
            break;
        }

        case WM_COMMAND: {
            // DEBUG_PRINTF("WM_COMMAND wparam %#x, lparam  %#x\n", wParam, lParam);
            if (HIWORD(wParam) == 0) {
                switch (LOWORD(wParam)) {
                    case IDC_AUTO_TRAY_ITEM_UPDATE: {
                        autoTrayListViewItemUpdate(hwndDlg, autoTrayListViewActiveItem_);
                        break;
                    }

                    case IDC_AUTO_TRAY_ITEM_ADD: {
                        autoTrayListViewItemAdd(hwndDlg);
                        break;
                    }

                    case IDC_AUTO_TRAY_ITEM_DELETE: {
                        autoTrayListViewItemDelete(hwndDlg, autoTrayListViewActiveItem_);
                        break;
                    }

                    case IDOK: {
                        DEBUG_PRINTF("Settings dialog done, updating settings\n");

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

                        settings_.autoTrays_ = autoTrayListViewGetItems(hwndDlg);

                        EndDialog(hwndDlg, wParam);

                        if (completionCallback_) {
                            completionCallback_(true, settings_);
                        }

                        return TRUE;
                    }

                    case IDCANCEL: {
                        DEBUG_PRINTF("Settings dialog cancelled\n");

                        EndDialog(hwndDlg, wParam);

                        if (completionCallback_) {
                            completionCallback_(false, settings_);
                        }

                        return TRUE;
                    }

                    default: {
                        DEBUG_PRINTF("WM_COMMAND %#x\n", wParam);
                        break;
                    }
                }
            }
            break;
        }

        default: {
            // DEBUG_PRINTF("message %#x, param %#x\n", message, wParam);
            break;
        }
    }

    return FALSE;
}

void autoTrayListViewInit(HWND hwndDlg)
{
    autoTrayListView_ = GetDlgItem(hwndDlg, IDC_AUTO_TRAY_LIST);

    unsigned int styles = LVS_EX_FULLROWSELECT; // | LVS_EX_GRIDLINES;
    if (SendMessageA(autoTrayListView_, LVM_SETEXTENDEDLISTVIEWSTYLE, styles, styles) == -1) {
        DEBUG_PRINTF("SendMessage LVM_SETEXTENDEDLISTVIEWSTYLE failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    CHAR szText[256];

    LVCOLUMNA listViewColumn;
    memset(&listViewColumn, 0, sizeof(listViewColumn));
    listViewColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    listViewColumn.fmt = LVCFMT_LEFT;
    listViewColumn.pszText = szText;

    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);

    listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::Executable);
    listViewColumn.cx = 200;
    LoadStringA(hInstance, IDS_COLUMN_EXECUTABLE, szText, sizeof(szText) / sizeof(szText[0]));
    if (SendMessageA(autoTrayListView_, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) == -1) {
        DEBUG_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    ++listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowClass);
    listViewColumn.cx = 200;
    LoadStringA(hInstance, IDS_COLUMN_WINDOW_CLASS, szText, sizeof(szText) / sizeof(szText[0]));
    if (SendMessageA(autoTrayListView_, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) == -1) {
        DEBUG_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowTitle);
    listViewColumn.cx = 200;
    LoadStringA(hInstance, IDS_COLUMN_WINDOW_TITLE, szText, sizeof(szText) / sizeof(szText[0]));
    if (SendMessageA(autoTrayListView_, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) == -1) {
        DEBUG_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    if (SendMessageA(autoTrayListView_, LVM_SETITEMCOUNT, (WPARAM)settings_.autoTrays_.size(), 0) == -1) {
        DEBUG_PRINTF("SendMessage LVM_SETITEMCOUNT failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_TEXT | LVIF_PARAM;

    for (size_t a = 0; a < settings_.autoTrays_.size(); ++a) {
        listViewItem.iItem = (int)a;
        listViewItem.lParam = a;
        listViewItem.iSubItem = (int)AutoTrayListViewColumn::Executable;
        listViewItem.pszText = (LPSTR)settings_.autoTrays_[a].executable_.c_str();
        if (SendMessageA(autoTrayListView_, LVM_INSERTITEMA, 0, (LPARAM)&listViewItem) == -1) {
            DEBUG_PRINTF("SendMessage LVM_INSERTITEMA failed: %s\n", StringUtility::lastErrorString().c_str());
            errorMessage(IDS_ERROR_CREATE_DIALOG);
            return;
        }
        listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowClass;
        listViewItem.pszText = (LPSTR)settings_.autoTrays_[a].windowClass_.c_str();
        if (SendMessageA(autoTrayListView_, LVM_SETITEMTEXTA, a, (LPARAM)&listViewItem) == -1) {
            DEBUG_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
            errorMessage(IDS_ERROR_CREATE_DIALOG);
            return;
        }
        listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowTitle;
        listViewItem.pszText = (LPSTR)settings_.autoTrays_[a].windowTitle_.c_str();
        if (SendMessageA(autoTrayListView_, LVM_SETITEMTEXTA, a, (LPARAM)&listViewItem) == -1) {
            DEBUG_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
            errorMessage(IDS_ERROR_CREATE_DIALOG);
            return;
        }
    }

    autoTrayListViewActiveItem_ = -1;
#ifdef SORT_ENABLED
    autoTrayListViewSortAscending_ = true;
    autoTrayListViewSortColumn_ = 0;
#endif

    autoTrayListViewUpdateButtons(hwndDlg);
}

std::vector<Settings::AutoTray> autoTrayListViewGetItems(HWND /* hwndDlg */)
{
    std::vector<Settings::AutoTray> autoTrays;

    int itemCount = (int)SendMessageA(autoTrayListView_, LVM_GETITEMCOUNT, 0, 0);

    char str[256];
    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_TEXT;
    listViewItem.pszText = str;
    listViewItem.cchTextMax = sizeof(str);
    for (int item = 0; item < itemCount; ++item) {
        Settings::AutoTray autoTray;

        listViewItem.iItem = item;
        listViewItem.iSubItem = (int)AutoTrayListViewColumn::Executable;
        SendMessage(autoTrayListView_, LVM_GETITEMTEXTA, item, (LPARAM)&listViewItem);
        autoTray.executable_ = str;

        listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowClass;
        SendMessage(autoTrayListView_, LVM_GETITEMTEXTA, item, (LPARAM)&listViewItem);
        autoTray.windowClass_ = str;

        listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowTitle;
        SendMessage(autoTrayListView_, LVM_GETITEMTEXTA, item, (LPARAM)&listViewItem);
        autoTray.windowTitle_ = str;

        if (!autoTray.executable_.empty() || !autoTray.windowClass_.empty() || !autoTray.windowTitle_.empty()) {
            autoTrays.push_back(autoTray);
        }
    }

    return autoTrays;
}

void autoTrayListViewNotify(HWND hwndDlg, LPNMHDR nmhdr)
{
    switch (nmhdr->code) {
        case NM_KILLFOCUS: {
            DEBUG_PRINTF("NM_KILLFOCUS\n");
            if (nmhdr->hwndFrom == GetDlgItem(hwndDlg, IDC_AUTO_TRAY_LIST)) {
                DEBUG_PRINTF("NM_KILLFOCUS matched\n");
                // FIX
                // autoTrayListViewActiveItem_ = -1;
                // autoTrayListViewUpdateSelected(hwndDlg);
                // autoTrayListViewUpdateButtons(hwndDlg);
            }
            break;
        }

        case LVN_COLUMNCLICK: {
#ifdef SORT_ENABLED
            NMLISTVIEW * pListView = (NMLISTVIEW *)nmhdr;

            if (pListView->iSubItem == autoTrayListViewSortColumn_) {
                autoTrayListViewSortAscending_ = !autoTrayListViewSortAscending_;
            }
            autoTrayListViewSortColumn_ = pListView->iSubItem;
            int sortColumn =
                (autoTrayListViewSortAscending_ ? (autoTrayListViewSortColumn_ + 1) : -(autoTrayListViewSortColumn_ + 1));

            DEBUG_PRINTF("SORTING COLUMN %d %s\n", sortColumn, autoTrayListViewSortAscending_ ? "ASC" : "DESC");
            if (!SendMessageA(autoTrayListView_, LVM_SORTITEMS, (WPARAM)sortColumn, (LPARAM)autoTrayListViewCompare)) {
                DEBUG_PRINTF("SendMessage LVM_SORTITEMS failed: %s\n", StringUtility::lastErrorString().c_str());
            }
#endif
            break;
        }

        case NM_CLICK:
        case LVN_ITEMACTIVATE: {
            DEBUG_PRINTF("LVN_ITEMACTIVATE\n");
            LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)nmhdr;
            autoTrayListViewItemEdit(hwndDlg, lpnmia->iItem);
            break;
        }

        case LVN_DELETEALLITEMS: {
            DEBUG_PRINTF("LVN_DELETEALLITEMS\n");
            break;
        }

            // case NM_CLICK: {
            //     LPNMITEMACTIVATE nmia = (LPNMITEMACTIVATE)nmhdr;
            //     DEBUG_PRINTF("NM_CLICK %d\n", nmia->iItem);
            //     autoTrayListViewActiveItem_ = nmia->iItem;
            //     autoTrayListViewUpdateButtons(hwndDlg);
            //     break;
            // }

        default: {
            // DEBUG_PRINTF("WM_NOTIFY default %#x\n", nmhdr->code);
            break;
        }
    }
}

#ifdef SORT_ENABLED
int autoTrayListViewCompare(LPARAM item1, LPARAM item2, LPARAM sortParam)
{
    int sortColumn = abs((int)sortParam) - 1;

    char str1[256];
    LVITEMA listViewItem1;
    memset(&listViewItem1, 0, sizeof(listViewItem1));
    listViewItem1.mask = LVIF_TEXT;
    listViewItem1.iItem = (int)item1;
    listViewItem1.iSubItem = sortColumn;
    listViewItem1.pszText = str1;
    listViewItem1.cchTextMax = sizeof(str1);
    SendMessage(autoTrayListView_, LVM_GETITEMTEXTA, item1, (LPARAM)&listViewItem1);

    char str2[256];
    LVITEMA listViewItem2;
    memset(&listViewItem2, 0, sizeof(listViewItem2));
    listViewItem2.mask = LVIF_TEXT;
    listViewItem2.iItem = (int)item2;
    listViewItem2.iSubItem = sortColumn;
    listViewItem2.pszText = str2;
    listViewItem2.cchTextMax = sizeof(str2);
    SendMessage(autoTrayListView_, LVM_GETITEMTEXTA, item2, (LPARAM)&listViewItem2);

    int ret = (sortParam > 0) ? strcmp(str1, str2) : strcmp(str2, str1);
    DEBUG_PRINTF(
        "autoTrayListViewCompare item1 %ld (%s) item2 %ld (%s) sort %ld (col %d): ret %d\n",
        item1,
        str1,
        item2,
        str2,
        sortParam,
        sortColumn,
        ret);
    return ret;
}
#endif

void autoTrayListViewItemAdd(HWND hwndDlg)
{
    DEBUG_PRINTF("Deleting auto tray item\n");

    int itemCount = (int)SendMessageA(autoTrayListView_, LVM_GETITEMCOUNT, 0, 0);

    char str[256];

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_TEXT | LVIF_PARAM;
    listViewItem.iItem = itemCount;
    listViewItem.lParam = itemCount;

    GetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_EXECUTABLE), str, sizeof(str));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::Executable;
    listViewItem.pszText = str;
    if (SendMessageA(autoTrayListView_, LVM_INSERTITEMA, 0, (LPARAM)&listViewItem) == -1) {
        DEBUG_PRINTF("SendMessage LVM_INSERTITEMA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    GetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_WINDOWCLASS), str, sizeof(str));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowClass;
    listViewItem.pszText = str;
    if (SendMessageA(autoTrayListView_, LVM_SETITEMTEXTA, itemCount, (LPARAM)&listViewItem) == -1) {
        DEBUG_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    GetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_WINDOWTITLE), str, sizeof(str));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowTitle;
    listViewItem.pszText = str;
    if (SendMessageA(autoTrayListView_, LVM_SETITEMTEXTA, itemCount, (LPARAM)&listViewItem) == -1) {
        DEBUG_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    autoTrayListViewItemEdit(hwndDlg, itemCount);

    autoTrayListViewActiveItem_ = itemCount;
    autoTrayListViewUpdateButtons(hwndDlg);
    autoTrayListViewUpdateSelected(hwndDlg);
}

void autoTrayListViewItemUpdate(HWND hwndDlg, int item)
{
    DEBUG_PRINTF("Updating auto tray item %d\n", item);

    int itemCount = (int)SendMessageA(autoTrayListView_, LVM_GETITEMCOUNT, 0, 0);

    if ((item < 0) || (item >= itemCount)) {
        DEBUG_PRINTF("Item %d out of range\n", item);
        return;
    }

    char str[256];

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_TEXT | LVIF_PARAM;
    listViewItem.iItem = itemCount;
    listViewItem.lParam = itemCount;

    GetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_EXECUTABLE), str, sizeof(str));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::Executable;
    listViewItem.pszText = str;
    if (SendMessageA(autoTrayListView_, LVM_SETITEMTEXTA, item, (LPARAM)&listViewItem) == -1) {
        DEBUG_PRINTF("SendMessage LVM_INSERTITEMA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    GetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_WINDOWCLASS), str, sizeof(str));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowClass;
    listViewItem.pszText = str;
    if (SendMessageA(autoTrayListView_, LVM_SETITEMTEXTA, item, (LPARAM)&listViewItem) == -1) {
        DEBUG_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    GetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_WINDOWTITLE), str, sizeof(str));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowTitle;
    listViewItem.pszText = str;
    if (SendMessageA(autoTrayListView_, LVM_SETITEMTEXTA, item, (LPARAM)&listViewItem) == -1) {
        DEBUG_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }
}

void autoTrayListViewItemDelete(HWND hwndDlg, int item)
{
    DEBUG_PRINTF("Deleting auto tray item %d\n", item);

    int itemCount = (int)SendMessageA(autoTrayListView_, LVM_GETITEMCOUNT, 0, 0);

    if ((item < 0) || (item >= itemCount)) {
        DEBUG_PRINTF("Item %d out of range\n", item);
        return;
    }

    if (!SendMessageA(autoTrayListView_, LVM_DELETEITEM, item, 0)) {
        DEBUG_PRINTF("SendMessage LVM_DELETEITEM failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    itemCount = (int)SendMessageA(autoTrayListView_, LVM_GETITEMCOUNT, 0, 0);

    if (autoTrayListViewActiveItem_ >= itemCount) {
        autoTrayListViewActiveItem_ = itemCount - 1;
    }
    autoTrayListViewUpdateButtons(hwndDlg);
    autoTrayListViewUpdateSelected(hwndDlg);
}

void autoTrayListViewItemEdit(HWND hwndDlg, int item)
{
    DEBUG_PRINTF("Editing auto tray item %d\n", item);

    int itemCount = (int)SendMessageA(autoTrayListView_, LVM_GETITEMCOUNT, 0, 0);

    if ((item < 0) || (item >= itemCount)) {
        DEBUG_PRINTF("Item %d out of range\n", item);
        SetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_EXECUTABLE), "");
        SetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_WINDOWCLASS), "");
        SetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_WINDOWTITLE), "");
        autoTrayListViewActiveItem_ = -1;
    } else {
        char str[256];
        LVITEMA listViewItem;
        memset(&listViewItem, 0, sizeof(listViewItem));
        listViewItem.mask = LVIF_TEXT;
        listViewItem.pszText = str;
        listViewItem.cchTextMax = sizeof(str);
        listViewItem.iItem = item;

        listViewItem.iSubItem = (int)AutoTrayListViewColumn::Executable;
        SendMessage(autoTrayListView_, LVM_GETITEMTEXTA, item, (LPARAM)&listViewItem);
        SetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_EXECUTABLE), str);

        listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowClass;
        SendMessage(autoTrayListView_, LVM_GETITEMTEXTA, item, (LPARAM)&listViewItem);
        SetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_WINDOWCLASS), str);

        listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowTitle;
        SendMessage(autoTrayListView_, LVM_GETITEMTEXTA, item, (LPARAM)&listViewItem);
        SetWindowTextA(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_EDIT_WINDOWTITLE), str);

        autoTrayListViewActiveItem_ = item;
    }

    autoTrayListViewUpdateButtons(hwndDlg);
}

void autoTrayListViewUpdateButtons(HWND hwndDlg)
{
    DEBUG_PRINTF("Updating buttons\n");

    int itemCount = (int)SendMessageA(autoTrayListView_, LVM_GETITEMCOUNT, 0, 0);

    bool hasActiveItem = (autoTrayListViewActiveItem_ >= 0) && (autoTrayListViewActiveItem_ < itemCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_ITEM_DELETE), hasActiveItem);
}

void autoTrayListViewUpdateSelected(HWND /*hwndDlg*/)
{
    DEBUG_PRINTF("Updating selected %d\n", autoTrayListViewActiveItem_);

    if (autoTrayListViewActiveItem_ == -1) {
        LVITEM listViewItem;
        memset(&listViewItem, 0, sizeof(listViewItem));
        listViewItem.mask = LVIF_STATE;
        listViewItem.state = 0;
        listViewItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        if (SendMessageA(autoTrayListView_, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&listViewItem) == -1) {
            DEBUG_PRINTF("SendMessage LVM_SETITEMSTATE failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    } else {
        if (!SetFocus(autoTrayListView_)) {
            DEBUG_PRINTF("SetFocus failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        if (SendMessageA(autoTrayListView_, LVM_ENSUREVISIBLE, autoTrayListViewActiveItem_, (LPARAM)TRUE) == -1) {
            DEBUG_PRINTF("SendMessage LVM_ENSUREVISIBLE failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        LVITEM listViewItem;
        memset(&listViewItem, 0, sizeof(listViewItem));
        listViewItem.mask = LVIF_STATE;
        listViewItem.state = LVIS_FOCUSED | LVIS_SELECTED;
        listViewItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        if (SendMessageA(autoTrayListView_, LVM_SETITEMSTATE, autoTrayListViewActiveItem_, (LPARAM)&listViewItem) == -1) {
            DEBUG_PRINTF("SendMessage LVM_SETITEMSTATE failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    }
}

} // namespace SettingsDialog
