// Copyright 2020 Benbuck Nason

#include "SettingsDialog.h"

// MinTray
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
static void autoTrayListViewNotify(HWND hwndDlg, LPNMHDR nmhdr);
#ifdef SORT_ENABLED
static int autoTrayListViewCompare(LPARAM, LPARAM, LPARAM);
#endif
static void autoTrayListViewItemAdd(HWND hwndDlg);
static void autoTrayListViewItemEdit(HWND hwndDlg, unsigned int item);
static void autoTrayListViewItemDelete(HWND hwndDlg, unsigned int item);
static void autoTrayListViewUpdateButtons(HWND hwndDlg);
static void autoTrayListViewUpdateSelected(HWND hwndDlg);

static Settings settings_;
static CompletionCallback completionCallback_;
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
                    case IDC_AUTO_TRAY_ITEM_ADD: {
                        autoTrayListViewItemAdd(hwndDlg);
                        break;
                    }

                    case IDC_AUTO_TRAY_ITEM_EDIT: {
                        autoTrayListViewItemEdit(hwndDlg, autoTrayListViewActiveItem_);
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
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);
    HWND autoTrayList = GetDlgItem(hwndDlg, IDC_AUTO_TRAY_LIST);
    CHAR szText[256];

    unsigned int styles = LVS_EX_FULLROWSELECT; // | LVS_EX_GRIDLINES;
    if (SendMessageA(autoTrayList, LVM_SETEXTENDEDLISTVIEWSTYLE, styles, styles) == -1) {
        DEBUG_PRINTF("SendMessage LVM_SETEXTENDEDLISTVIEWSTYLE failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    LVCOLUMNA listViewColumn;
    memset(&listViewColumn, 0, sizeof(listViewColumn));
    listViewColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    listViewColumn.fmt = LVCFMT_LEFT;
    listViewColumn.pszText = szText;

    listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::Executable);
    listViewColumn.cx = 250;
    LoadStringA(hInstance, IDS_COLUMN_EXECUTABLE, szText, sizeof(szText) / sizeof(szText[0]));
    if (SendMessageA(autoTrayList, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) == -1) {
        DEBUG_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    ++listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowClass);
    listViewColumn.cx = 147;
    LoadStringA(hInstance, IDS_COLUMN_WINDOW_CLASS, szText, sizeof(szText) / sizeof(szText[0]));
    if (SendMessageA(autoTrayList, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) == -1) {
        DEBUG_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowTitle);
    listViewColumn.cx = 175;
    LoadStringA(hInstance, IDS_COLUMN_WINDOW_TITLE, szText, sizeof(szText) / sizeof(szText[0]));
    if (SendMessageA(autoTrayList, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) == -1) {
        DEBUG_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    if (SendMessageA(autoTrayList, LVM_SETITEMCOUNT, (WPARAM)settings_.autoTrays_.size(), 0) == -1) {
        DEBUG_PRINTF("SendMessage LVM_SETITEMCOUNT failed: %s\n", StringUtility::lastErrorString().c_str());
        errorMessage(IDS_ERROR_CREATE_DIALOG);
        return;
    }

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_TEXT | LVIF_PARAM;
    listViewItem.pszText = LPSTR_TEXTCALLBACKA; // sends an LVN_GETDISPINFO message to get item contents

    for (size_t a = 0; a < settings_.autoTrays_.size(); ++a) {
        listViewItem.iItem = (int)a;
        listViewItem.lParam = a;
        if (SendMessageA(autoTrayList, LVM_INSERTITEMA, 0, (LPARAM)&listViewItem) == -1) {
            DEBUG_PRINTF("SendMessage LVM_INSERTITEMA failed: %s\n", StringUtility::lastErrorString().c_str());
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

        case LVN_GETDISPINFOA: {
            NMLVDISPINFOA * displayInfo = (NMLVDISPINFOA *)nmhdr;
            // DEBUG_PRINTF("LVN_GETDISPINFOA item %d %d\n", displayInfo->item.iItem, displayInfo->item.iSubItem);
            const char * str = nullptr;
#ifdef SORT_ENABLED
            const Settings::AutoTray & autoTray = settings_.autoTrays_[displayInfo->item.lParam];
#else
            const Settings::AutoTray & autoTray = settings_.autoTrays_[displayInfo->item.iItem];
#endif
            switch (static_cast<AutoTrayListViewColumn>(displayInfo->item.iSubItem)) {
                case AutoTrayListViewColumn::Executable: str = autoTray.executable_.c_str(); break;
                case AutoTrayListViewColumn::WindowClass: str = autoTray.windowClass_.c_str(); break;
                case AutoTrayListViewColumn::WindowTitle: str = autoTray.windowTitle_.c_str(); break;

                default: {
                    __debugbreak();
                    str = "";
                    break;
                }
            }
            displayInfo->item.pszText = const_cast<CHAR *>(str);
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

            HWND autoTrayList = GetDlgItem(hwndDlg, IDC_AUTO_TRAY_LIST);
            if (SendMessageA(autoTrayList, LVM_SORTITEMS, (WPARAM)sortColumn, (LPARAM)autoTrayListViewCompare)) {
                DEBUG_PRINTF("SendMessage LVM_SORTITEMS failed: %s\n", StringUtility::lastErrorString().c_str());
                break;
            }
#endif
            break;
        }

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

        case NM_CLICK: {
            NMITEMACTIVATE * nmia = (NMITEMACTIVATE *)nmhdr;
            DEBUG_PRINTF("NM_CLICK %d\n", nmia->iItem);
            autoTrayListViewActiveItem_ = nmia->iItem;
            autoTrayListViewUpdateButtons(hwndDlg);
            break;
        }

        default: {
            // DEBUG_PRINTF("WM_NOTIFY default %#x\n", nmhdr->code);
            break;
        }
    }
}

#ifdef SORT_ENABLED
int autoTrayListViewCompare(LPARAM item1, LPARAM item2, LPARAM sortParam)
{
    // DEBUG_PRINTF("autoTrayListViewCompare item1 %ld item2 %ld sort %ld\n", item1, item2, sortParam);
    int sortColumn = abs((int)sortParam) - 1;

    const char * str1;
    const char * str2;
    switch (static_cast<AutoTrayListViewColumn>(sortColumn)) {
        case AutoTrayListViewColumn::Executable: {
            str1 = settings_.autoTrays_[item1].executable_.c_str();
            str2 = settings_.autoTrays_[item2].executable_.c_str();
            break;
        }

        case AutoTrayListViewColumn::WindowClass: {
            str1 = settings_.autoTrays_[item1].windowClass_.c_str();
            str2 = settings_.autoTrays_[item2].windowClass_.c_str();
            break;
        }

        case AutoTrayListViewColumn::WindowTitle: {
            str1 = settings_.autoTrays_[item1].windowTitle_.c_str();
            str2 = settings_.autoTrays_[item2].windowTitle_.c_str();
            break;
        }

        default: {
            __debugbreak();
            return -1;
        }
    }

    return (sortParam > 0) ? strcmp(str1, str2) : strcmp(str2, str1);
}
#endif

void autoTrayListViewItemAdd(HWND hwndDlg)
{
    DEBUG_PRINTF("Deleting auto tray item\n");

    HWND autoTrayList = GetDlgItem(hwndDlg, IDC_AUTO_TRAY_LIST);

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.iItem = (int)settings_.autoTrays_.size();
    listViewItem.mask = LVIF_TEXT | LVIF_PARAM;
    listViewItem.pszText = LPSTR_TEXTCALLBACKA; // sends an LVN_GETDISPINFO message to get item contents
    listViewItem.lParam = settings_.autoTrays_.size();
    if (SendMessageA(autoTrayList, LVM_INSERTITEMA, 0, (LPARAM)&listViewItem) == -1) {
        DEBUG_PRINTF("SendMessage LVM_INSERTITEMA failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    settings_.addAutoTray("", "", "");

    autoTrayListViewItemEdit(hwndDlg, (int)settings_.autoTrays_.size() - 1);

    autoTrayListViewActiveItem_ = (int)settings_.autoTrays_.size() - 1;
    autoTrayListViewUpdateButtons(hwndDlg);
    autoTrayListViewUpdateSelected(hwndDlg);
}

void autoTrayListViewItemEdit(HWND hwndDlg, unsigned int item)
{
    DEBUG_PRINTF("Editing auto tray item %d\n", item);

    // FIX - do actual editing
    MessageBoxA(hwndDlg, (std::string("Edit ") + std::to_string(item)).c_str(), "Edit", MB_OK);

    autoTrayListViewUpdateSelected(hwndDlg);
}

void autoTrayListViewItemDelete(HWND hwndDlg, unsigned int item)
{
    DEBUG_PRINTF("Deleting auto tray item %d\n", item);

    HWND autoTrayList = GetDlgItem(hwndDlg, IDC_AUTO_TRAY_LIST);

    if (SendMessageA(autoTrayList, LVM_DELETEITEM, item, 0) == -1) {
        DEBUG_PRINTF("SendMessage LVM_DELETEITEM failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    settings_.autoTrays_.erase(settings_.autoTrays_.begin() + item);

    if (autoTrayListViewActiveItem_ >= (int)settings_.autoTrays_.size()) {
        autoTrayListViewActiveItem_ = (int)settings_.autoTrays_.size() - 1;
    }
    autoTrayListViewUpdateButtons(hwndDlg);
    autoTrayListViewUpdateSelected(hwndDlg);
}

void autoTrayListViewUpdateButtons(HWND hwndDlg)
{
    DEBUG_PRINTF("Updating buttons\n");

    bool hasActiveItem = (autoTrayListViewActiveItem_ >= 0) &&
        (autoTrayListViewActiveItem_ < (int)settings_.autoTrays_.size());
    EnableWindow(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_ITEM_EDIT), hasActiveItem);
    EnableWindow(GetDlgItem(hwndDlg, IDC_AUTO_TRAY_ITEM_DELETE), hasActiveItem);
}

void autoTrayListViewUpdateSelected(HWND hwndDlg)
{
    DEBUG_PRINTF("Updating selected %d\n", autoTrayListViewActiveItem_);

    HWND autoTrayList = GetDlgItem(hwndDlg, IDC_AUTO_TRAY_LIST);

    if (autoTrayListViewActiveItem_ == -1) {
        LVITEM listViewItem;
        memset(&listViewItem, 0, sizeof(listViewItem));
        listViewItem.mask = LVIF_STATE;
        listViewItem.state = 0;
        listViewItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        if (SendMessageA(autoTrayList, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&listViewItem) == -1) {
            DEBUG_PRINTF("SendMessage LVM_SETITEMSTATE failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    } else {
        if (!SetFocus(autoTrayList)) {
            DEBUG_PRINTF("SetFocus failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        if (SendMessageA(autoTrayList, LVM_ENSUREVISIBLE, autoTrayListViewActiveItem_, (LPARAM)TRUE) == -1) {
            DEBUG_PRINTF("SendMessage LVM_ENSUREVISIBLE failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        LVITEM listViewItem;
        memset(&listViewItem, 0, sizeof(listViewItem));
        listViewItem.mask = LVIF_STATE;
        listViewItem.state = LVIS_FOCUSED | LVIS_SELECTED;
        listViewItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        if (SendMessageA(autoTrayList, LVM_SETITEMSTATE, autoTrayListViewActiveItem_, (LPARAM)&listViewItem) == -1) {
            DEBUG_PRINTF("SendMessage LVM_SETITEMSTATE failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    }
}

} // namespace SettingsDialog
