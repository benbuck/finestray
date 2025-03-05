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

#include "SettingsDialog.h"

// App
#include "AboutDialog.h"
#include "Helpers.h"
#include "Hotkey.h"
#include "Log.h"
#include "Resource.h"
#include "StringUtility.h"

// Windows
#include <CommCtrl.h>
#include <Psapi.h>
#include <Windows.h>
#include <shellapi.h>

// Standard library
#include <algorithm>
#include <string>
#include <vector>

// #define SORT_ENABLED

namespace
{

enum class AutoTrayListViewColumn
{
    Executable,
    WindowClass,
    WindowTitle,
    Count
};

INT_PTR settingsDialogFunc(HWND dialogHwnd, UINT message, WPARAM wParam, LPARAM lParam);
void autoTrayListViewInit(HWND dialogHwnd);
std::vector<Settings::AutoTray> autoTrayListViewGetItems(HWND dialogHwnd);
void autoTrayListViewNotify(HWND dialogHwnd, LPNMHDR nmhdr);
#ifdef SORT_ENABLED
int autoTrayListViewCompare(LPARAM, LPARAM, LPARAM);
#endif
void autoTrayListViewItemAdd(HWND dialogHwnd);
void autoTrayListViewItemUpdate(HWND dialogHwnd, int item);
void autoTrayListViewItemDelete(HWND dialogHwnd, int item);
void autoTrayListViewItemSpy(HWND dialogHwnd);
void autoTrayListViewItemEdit(HWND dialogHwnd, int item);
void autoTrayListViewUpdateButtons(HWND dialogHwnd);
void autoTrayListViewUpdateSelected(HWND dialogHwnd);
void spySelectWindowAtPoint(const POINT & point);
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
std::string getDialogItemText(HWND dialogHwnd, int id);
std::string getListViewItemText(HWND listViewHwnd, int item, int subItem);

Settings settings_;
SettingsDialog::CompletionCallback completionCallback_;
HWND autoTrayListViewHwnd_;
int autoTrayListViewActiveItem_;
#ifdef SORT_ENABLED
bool autoTrayListViewSortAscending_;
int autoTrayListViewSortColumn_;
#endif
bool spyMode_;
HWND spuModeHwnd_;
HHOOK mouseHhook_;

} // anonymous namespace

namespace SettingsDialog
{
HWND create(HWND hwnd, const Settings & settings, CompletionCallback completionCallback)
{
    settings_ = settings;
    completionCallback_ = completionCallback;

    HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(nullptr);
    HWND dialogHwnd = CreateDialogA(hinstance, MAKEINTRESOURCEA(IDD_DIALOG_SETTINGS), hwnd, settingsDialogFunc);

    ShowWindow(dialogHwnd, SW_SHOW);
    if (!SetForegroundWindow(dialogHwnd)) {
        WARNING_PRINTF("SetForegroundWindow failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    return dialogHwnd;
}

} // namespace SettingsDialog

namespace
{

INT_PTR settingsDialogFunc(HWND dialogHwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // DEBUG_PRINTF("wnd %#x, message %#x, wparam %#x, lparam %#x\n", dialogHwnd, message, wParam, lParam);

    if (spyMode_) {
        if (message == WM_LBUTTONDOWN) {
            POINT point;
            if (!GetCursorPos(&point)) {
                WARNING_PRINTF("GetCursorPos failed: %s\n", StringUtility::lastErrorString().c_str());
            } else {
                spySelectWindowAtPoint(point);
            }
        }

        return FALSE;
    }

    switch (message) {
        case WM_INITDIALOG: {
            if (!CheckDlgButton(
                    dialogHwnd,
                    IDC_START_WITH_WINDOWS,
                    settings_.startWithWindows_ ? BST_CHECKED : BST_UNCHECKED)) {
                WARNING_PRINTF("CheckDlgButton failed: %s\n", StringUtility::lastErrorString().c_str());
            }

            if (!CheckDlgButton(dialogHwnd, IDC_LOG_TO_FILE, settings_.logToFile_ ? BST_CHECKED : BST_UNCHECKED)) {
                WARNING_PRINTF("CheckDlgButton failed: %s\n", StringUtility::lastErrorString().c_str());
            }

            int checkButtonId = ICC_MINIMIZE_PLACEMENT_TRAY_AND_MENU;
            switch (settings_.minimizePlacement_) {
                case MinimizePlacement::Tray: checkButtonId = IDC_MINIMIZE_PLACEMENT_TRAY; break;
                case MinimizePlacement::Menu: checkButtonId = IDC_MINIMIZE_PLACEMENT_MENU; break;
                case MinimizePlacement::TrayAndMenu: checkButtonId = ICC_MINIMIZE_PLACEMENT_TRAY_AND_MENU; break;
                case MinimizePlacement::None:
                default: {
                    WARNING_PRINTF("bad minimize placement %d\n", settings_.minimizePlacement_);
                    break;
                }
            }

            if (!CheckRadioButton(
                    dialogHwnd,
                    IDC_MINIMIZE_PLACEMENT_TRAY,
                    ICC_MINIMIZE_PLACEMENT_TRAY_AND_MENU,
                    checkButtonId)) {
                WARNING_PRINTF("CheckRadioButton failed: %s\n", StringUtility::lastErrorString().c_str());
            }

            if (!SetDlgItemTextA(dialogHwnd, IDC_HOTKEY_MINIMIZE, settings_.hotkeyMinimize_.c_str())) {
                WARNING_PRINTF("SetDlgItemText failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_HOTKEY_RESTORE, settings_.hotkeyRestore_.c_str())) {
                WARNING_PRINTF("SetDlgItemText failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_HOTKEY_RESTORE_ALL, settings_.hotkeyRestoreAll_.c_str())) {
                WARNING_PRINTF("SetDlgItemText failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_HOTKEY_MENU, settings_.hotkeyMenu_.c_str())) {
                WARNING_PRINTF("SetDlgItemText failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_MODIFIER_OVERRIDE, settings_.modifiersOverride_.c_str())) {
                WARNING_PRINTF("SetDlgItemText failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_POLL_INTERVAL, std::to_string(settings_.pollInterval_).c_str())) {
                WARNING_PRINTF("SetDlgItemText failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, "")) {
                WARNING_PRINTF("SetWindowTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, "")) {
                WARNING_PRINTF("SetWindowTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, "")) {
                WARNING_PRINTF("SetWindowTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }

            autoTrayListViewInit(dialogHwnd);

            break;
        }

        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            // DEBUG_PRINTF("nmhdr hwnd %#x, id %#x, code %d\n", nmhdr->hwndFrom, nmhdr->idFrom, (int)nmhdr->code);
            if (nmhdr->hwndFrom == GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_LIST)) {
                autoTrayListViewNotify(dialogHwnd, nmhdr);
            }
            break;
        }

        case WM_COMMAND: {
            // DEBUG_PRINTF("WM_COMMAND wparam %#x, lparam  %#x\n", wParam, lParam);
            if (HIWORD(wParam) == 0) {
                switch (LOWORD(wParam)) {
                    case IDC_START_WITH_WINDOWS: {
                        settings_.startWithWindows_ = IsDlgButtonChecked(dialogHwnd, IDC_START_WITH_WINDOWS) ==
                            BST_CHECKED;
                        break;
                    }

                    case IDC_LOG_TO_FILE: {
                        settings_.logToFile_ = IsDlgButtonChecked(dialogHwnd, IDC_LOG_TO_FILE) == BST_CHECKED;
                        break;
                    }

                    case IDC_MINIMIZE_PLACEMENT_TRAY: {
                        settings_.minimizePlacement_ = MinimizePlacement::Tray;
                        break;
                    }

                    case IDC_MINIMIZE_PLACEMENT_MENU: {
                        settings_.minimizePlacement_ = MinimizePlacement::Menu;
                        break;
                    }

                    case ICC_MINIMIZE_PLACEMENT_TRAY_AND_MENU: {
                        settings_.minimizePlacement_ = MinimizePlacement::TrayAndMenu;
                        break;
                    }

                    case IDC_AUTO_TRAY_ITEM_UPDATE: {
                        autoTrayListViewItemUpdate(dialogHwnd, autoTrayListViewActiveItem_);
                        break;
                    }

                    case IDC_AUTO_TRAY_ITEM_ADD: {
                        autoTrayListViewItemAdd(dialogHwnd);
                        break;
                    }

                    case IDC_AUTO_TRAY_ITEM_DELETE: {
                        autoTrayListViewItemDelete(dialogHwnd, autoTrayListViewActiveItem_);
                        break;
                    }
                    case IDC_AUTO_TRAY_ITEM_SPY: {
                        autoTrayListViewItemSpy(dialogHwnd);
                        break;
                    }

                    case IDC_HELP_PAGE: {
                        INFO_PRINTF("Opening help page\n");
                        ShellExecuteA(
                            nullptr,
                            "open",
                            "https://github.com/benbuck/finestray/blob/main/README.md",
                            nullptr,
                            nullptr,
                            SW_SHOWNORMAL);
                        break;
                    }

                    case IDC_ABOUT: {
                        showAboutDialog(dialogHwnd);
                        break;
                    }

                    case IDC_RESET: {
                        settings_ = Settings();
                        SendMessageA(dialogHwnd, WM_INITDIALOG, 0, 0);
                        break;
                    }

                    case IDC_EXIT:
                    case IDOK: {
                        INFO_PRINTF("Settings dialog done, updating settings\n");

                        settings_.hotkeyMinimize_ = getDialogItemText(dialogHwnd, IDC_HOTKEY_MINIMIZE);
                        settings_.hotkeyRestore_ = getDialogItemText(dialogHwnd, IDC_HOTKEY_RESTORE);
                        settings_.hotkeyRestoreAll_ = getDialogItemText(dialogHwnd, IDC_HOTKEY_RESTORE_ALL);
                        settings_.hotkeyMenu_ = getDialogItemText(dialogHwnd, IDC_HOTKEY_MENU);
                        settings_.modifiersOverride_ = getDialogItemText(dialogHwnd, IDC_MODIFIER_OVERRIDE);
                        settings_.pollInterval_ = std::stoul(getDialogItemText(dialogHwnd, IDC_POLL_INTERVAL));
                        settings_.autoTrays_ = autoTrayListViewGetItems(dialogHwnd);

                        EndDialog(dialogHwnd, wParam);

                        if (completionCallback_) {
                            completionCallback_(true, settings_);
                        }

                        if (LOWORD(wParam == IDC_EXIT)) {
                            PostQuitMessage(0);
                        }

                        return TRUE;
                    }

                    case IDCANCEL: {
                        INFO_PRINTF("Settings dialog cancelled\n");

                        EndDialog(dialogHwnd, wParam);

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

void autoTrayListViewInit(HWND dialogHwnd)
{
    autoTrayListViewHwnd_ = GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_LIST);

    HWND hWndHdr = (HWND)::SendMessage(autoTrayListViewHwnd_, LVM_GETHEADER, 0, 0);
    int count = (int)::SendMessage(hWndHdr, HDM_GETITEMCOUNT, 0, 0L);
    if (count < 3) {
        unsigned int styles = LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_GRIDLINES;
        if (SendMessageA(autoTrayListViewHwnd_, LVM_SETEXTENDEDLISTVIEWSTYLE, styles, styles) == -1) {
            std::string lastErrorString = StringUtility::lastErrorString();
            WARNING_PRINTF("SendMessage LVM_SETEXTENDEDLISTVIEWSTYLE failed: %s\n", lastErrorString.c_str());
            errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
            return;
        }

        RECT rect;
        GetWindowRect(autoTrayListViewHwnd_, &rect);
        int width = rect.right - rect.left;
        int columnWidth = (width - 18) / (int)AutoTrayListViewColumn::Count;

        LVCOLUMNA listViewColumn;
        memset(&listViewColumn, 0, sizeof(listViewColumn));
        listViewColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        listViewColumn.fmt = LVCFMT_LEFT;
        listViewColumn.cx = columnWidth;

        std::string str;

        listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::Executable);
        str = getResourceString(IDS_COLUMN_EXECUTABLE);
        listViewColumn.pszText = (LPSTR)str.c_str();
        if (SendMessageA(autoTrayListViewHwnd_, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) ==
            -1) {
            std::string lastErrorString = StringUtility::lastErrorString();
            WARNING_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", lastErrorString.c_str());
            errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
            return;
        }

        ++listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowClass);
        str = getResourceString(IDS_COLUMN_WINDOW_CLASS);
        listViewColumn.pszText = (LPSTR)str.c_str();
        if (SendMessageA(autoTrayListViewHwnd_, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) ==
            -1) {
            std::string lastErrorString = StringUtility::lastErrorString();
            WARNING_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", lastErrorString.c_str());
            errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
            return;
        }

        listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowTitle);
        str = getResourceString(IDS_COLUMN_WINDOW_TITLE);
        listViewColumn.pszText = (LPSTR)str.c_str();
        if (SendMessageA(autoTrayListViewHwnd_, LVM_INSERTCOLUMNA, listViewColumn.iSubItem, (LPARAM)&listViewColumn) ==
            -1) {
            std::string lastErrorString = StringUtility::lastErrorString();
            WARNING_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", lastErrorString.c_str());
            errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
            return;
        }
    }

    if (!SendMessageA(autoTrayListViewHwnd_, LVM_DELETEALLITEMS, 0, 0)) {
        WARNING_PRINTF("SendMessage LVM_DELETEALLITEMS failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMCOUNT, (WPARAM)settings_.autoTrays_.size(), 0) == -1) {
        std::string lastErrorString = StringUtility::lastErrorString();
        WARNING_PRINTF("SendMessage LVM_SETITEMCOUNT failed: %s\n", lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
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
        if (SendMessageA(autoTrayListViewHwnd_, LVM_INSERTITEMA, 0, (LPARAM)&listViewItem) == -1) {
            std::string lastErrorString = StringUtility::lastErrorString();
            WARNING_PRINTF("SendMessage LVM_INSERTITEMA failed: %s\n", lastErrorString.c_str());
            errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
            return;
        }
        listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowClass;
        listViewItem.pszText = (LPSTR)settings_.autoTrays_[a].windowClass_.c_str();
        if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, a, (LPARAM)&listViewItem) == -1) {
            std::string lastErrorString = StringUtility::lastErrorString();
            WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", lastErrorString.c_str());
            errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
            return;
        }
        listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowTitle;
        listViewItem.pszText = (LPSTR)settings_.autoTrays_[a].windowTitle_.c_str();
        if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, a, (LPARAM)&listViewItem) == -1) {
            std::string lastErrorString = StringUtility::lastErrorString();
            WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", lastErrorString.c_str());
            errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
            return;
        }
    }

    autoTrayListViewActiveItem_ = -1;
#ifdef SORT_ENABLED
    autoTrayListViewSortAscending_ = true;
    autoTrayListViewSortColumn_ = 0;
#endif

    autoTrayListViewUpdateButtons(dialogHwnd);
}

std::vector<Settings::AutoTray> autoTrayListViewGetItems(HWND /* dialogHwnd */)
{
    std::vector<Settings::AutoTray> autoTrays;

    int itemCount = (int)SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0);

    for (int item = 0; item < itemCount; ++item) {
        Settings::AutoTray autoTray;

        autoTray.executable_ = getListViewItemText(autoTrayListViewHwnd_, item, (int)AutoTrayListViewColumn::Executable);
        autoTray.windowClass_ = getListViewItemText(autoTrayListViewHwnd_, item, (int)AutoTrayListViewColumn::WindowClass);
        autoTray.windowTitle_ = getListViewItemText(autoTrayListViewHwnd_, item, (int)AutoTrayListViewColumn::WindowTitle);

        if (!autoTray.executable_.empty() || !autoTray.windowClass_.empty() || !autoTray.windowTitle_.empty()) {
            autoTrays.push_back(autoTray);
        }
    }

    return autoTrays;
}

void autoTrayListViewNotify(HWND dialogHwnd, LPNMHDR nmhdr)
{
    switch (nmhdr->code) {
        case LVN_COLUMNCLICK: {
#ifdef SORT_ENABLED
            LPNMLISTVIEW nmListView = (LPNMLISTVIEW)nmhdr;

            if (nmListView->iSubItem == autoTrayListViewSortColumn_) {
                autoTrayListViewSortAscending_ = !autoTrayListViewSortAscending_;
            }
            autoTrayListViewSortColumn_ = nmListView->iSubItem;
            int sortColumn =
                (autoTrayListViewSortAscending_ ? (autoTrayListViewSortColumn_ + 1) : -(autoTrayListViewSortColumn_ + 1));

            DEBUG_PRINTF("SORTING COLUMN %d %s\n", sortColumn, autoTrayListViewSortAscending_ ? "ASC" : "DESC");
            if (!SendMessageA(autoTrayListViewHwnd_, LVM_SORTITEMS, (WPARAM)sortColumn, (LPARAM)autoTrayListViewCompare)) {
                DEBUG_PRINTF("SendMessage LVM_SORTITEMS failed: %s\n", lastErrorString.c_str());
            }
#endif
            break;
        }

        case LVN_ITEMCHANGED: {
            LPNMLISTVIEW nmListView = (LPNMLISTVIEW)nmhdr;
            if (nmListView->uChanged & LVIF_STATE) {
                if (nmListView->uNewState & LVIS_SELECTED) {
                    autoTrayListViewItemEdit(dialogHwnd, nmListView->iItem);
                } else {
                    autoTrayListViewItemEdit(dialogHwnd, -1);
                }
            }
            break;
        }

        case LVN_DELETEALLITEMS: {
            DEBUG_PRINTF("LVN_DELETEALLITEMS\n");
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
    int sortColumn = abs((int)sortParam) - 1;

    std::string str1 = getListViewItemText(autoTrayListViewHwnd_, (int)item1, sortColumn);
    std::string str2 = getListViewItemText(autoTrayListViewHwnd_, (int)item2, sortColumn);

    int ret = (sortParam > 0) ? str1.compare(str2) : str2.compare(str1);
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

void autoTrayListViewItemAdd(HWND dialogHwnd)
{
    DEBUG_PRINTF("Deleting auto tray item\n");

    int itemCount = (int)SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0);

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_TEXT | LVIF_PARAM;
    listViewItem.iItem = itemCount;
    listViewItem.lParam = itemCount;

    std::string str;

    str = getWindowText(GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::Executable;
    listViewItem.pszText = (LPSTR)str.c_str();
    if (SendMessageA(autoTrayListViewHwnd_, LVM_INSERTITEMA, 0, (LPARAM)&listViewItem) == -1) {
        std::string lastErrorString = StringUtility::lastErrorString();
        WARNING_PRINTF("SendMessage LVM_INSERTITEMA failed: %s\n", lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
        return;
    }

    str = getWindowText(GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowClass;
    listViewItem.pszText = (LPSTR)str.c_str();
    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, itemCount, (LPARAM)&listViewItem) == -1) {
        std::string lastErrorString = StringUtility::lastErrorString();
        WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
        return;
    }

    str = getWindowText(GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowTitle;
    listViewItem.pszText = (LPSTR)str.c_str();
    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, itemCount, (LPARAM)&listViewItem) == -1) {
        std::string lastErrorString = StringUtility::lastErrorString();
        WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
        return;
    }

    autoTrayListViewItemEdit(dialogHwnd, itemCount);

    autoTrayListViewActiveItem_ = itemCount;
    autoTrayListViewUpdateButtons(dialogHwnd);
    autoTrayListViewUpdateSelected(dialogHwnd);
}

void autoTrayListViewItemUpdate(HWND dialogHwnd, int item)
{
    DEBUG_PRINTF("Updating auto tray item %d\n", item);

    int itemCount = (int)SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0);

    if ((item < 0) || (item >= itemCount)) {
        WARNING_PRINTF("Item %d out of range\n", item);
        return;
    }

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_TEXT | LVIF_PARAM;
    listViewItem.iItem = itemCount;
    listViewItem.lParam = itemCount;

    std::string str;

    str = getWindowText(GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::Executable;
    listViewItem.pszText = (LPSTR)str.c_str();
    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, item, (LPARAM)&listViewItem) == -1) {
        std::string lastErrorString = StringUtility::lastErrorString();
        WARNING_PRINTF("SendMessage LVM_INSERTITEMA failed: %s\n", lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
        return;
    }

    str = getWindowText(GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowClass;
    listViewItem.pszText = (LPSTR)str.c_str();
    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, item, (LPARAM)&listViewItem) == -1) {
        std::string lastErrorString = StringUtility::lastErrorString();
        WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
        return;
    }

    str = getWindowText(GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE));
    listViewItem.iSubItem = (int)AutoTrayListViewColumn::WindowTitle;
    listViewItem.pszText = (LPSTR)str.c_str();
    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, item, (LPARAM)&listViewItem) == -1) {
        std::string lastErrorString = StringUtility::lastErrorString();
        WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", lastErrorString.c_str());
        errorMessage(ErrorContext(IDS_ERROR_CREATE_DIALOG, lastErrorString));
        return;
    }
}

void autoTrayListViewItemDelete(HWND dialogHwnd, int item)
{
    DEBUG_PRINTF("Deleting auto tray item %d\n", item);

    int itemCount = (int)SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0);

    if ((item < 0) || (item >= itemCount)) {
        WARNING_PRINTF("Item %d out of range\n", item);
        return;
    }

    if (!SendMessageA(autoTrayListViewHwnd_, LVM_DELETEITEM, item, 0)) {
        WARNING_PRINTF("SendMessage LVM_DELETEITEM failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    itemCount = (int)SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0);

    if (autoTrayListViewActiveItem_ >= itemCount) {
        autoTrayListViewActiveItem_ = itemCount - 1;
    }
    autoTrayListViewUpdateButtons(dialogHwnd);
    autoTrayListViewUpdateSelected(dialogHwnd);
}

void autoTrayListViewItemSpy(HWND dialogHwnd)
{
    DEBUG_PRINTF("Spying auto tray\n");

    ShowWindow(dialogHwnd, SW_HIDE);

    MessageBoxA(
        dialogHwnd,
        getResourceString(IDS_SPY_MODE_TEXT).c_str(),
        getResourceString(IDS_SPY_MODE_TITLE).c_str(),
        MB_OK);

    mouseHhook_ = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, nullptr, 0);
    if (!mouseHhook_) {
        WARNING_PRINTF("Failed to install mouse hook!\n");
    } else {
        DEBUG_PRINTF("Mouse hook installed.\n");
    }

    spuModeHwnd_ = dialogHwnd;
    spyMode_ = true;
}

void autoTrayListViewItemEdit(HWND dialogHwnd, int item)
{
    DEBUG_PRINTF("Editing auto tray item %d\n", item);

    int itemCount = (int)SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0);

    if ((item < 0) || (item >= itemCount)) {
        WARNING_PRINTF("Item %d out of range\n", item);
        SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, "");
        SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, "");
        SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, "");
        autoTrayListViewActiveItem_ = -1;
    } else {
        std::string executable = getListViewItemText(autoTrayListViewHwnd_, item, (int)AutoTrayListViewColumn::Executable);
        SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, executable.c_str());

        std::string windowClass =
            getListViewItemText(autoTrayListViewHwnd_, item, (int)AutoTrayListViewColumn::WindowClass);
        SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, windowClass.c_str());

        std::string windowTitle =
            getListViewItemText(autoTrayListViewHwnd_, item, (int)AutoTrayListViewColumn::WindowTitle);
        SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, windowTitle.c_str());

        autoTrayListViewActiveItem_ = item;
    }

    autoTrayListViewUpdateButtons(dialogHwnd);
}

void autoTrayListViewUpdateButtons(HWND dialogHwnd)
{
    DEBUG_PRINTF("Updating buttons\n");

    int itemCount = (int)SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0);

    bool hasActiveItem = (autoTrayListViewActiveItem_ >= 0) && (autoTrayListViewActiveItem_ < itemCount);
    EnableWindow(GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_ITEM_DELETE), hasActiveItem);
}

void autoTrayListViewUpdateSelected(HWND /*dialogHwnd*/)
{
    DEBUG_PRINTF("Updating selected %d\n", autoTrayListViewActiveItem_);

    if (autoTrayListViewActiveItem_ == -1) {
        LVITEM listViewItem;
        memset(&listViewItem, 0, sizeof(listViewItem));
        listViewItem.mask = LVIF_STATE;
        listViewItem.state = 0;
        listViewItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&listViewItem) == -1) {
            WARNING_PRINTF("SendMessage LVM_SETITEMSTATE failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    } else {
        if (!SetFocus(autoTrayListViewHwnd_)) {
            WARNING_PRINTF("SetFocus failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        if (SendMessageA(autoTrayListViewHwnd_, LVM_ENSUREVISIBLE, autoTrayListViewActiveItem_, (LPARAM)TRUE) == -1) {
            WARNING_PRINTF("SendMessage LVM_ENSUREVISIBLE failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        LVITEM listViewItem;
        memset(&listViewItem, 0, sizeof(listViewItem));
        listViewItem.mask = LVIF_STATE;
        listViewItem.state = LVIS_FOCUSED | LVIS_SELECTED;
        listViewItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMSTATE, autoTrayListViewActiveItem_, (LPARAM)&listViewItem) ==
            -1) {
            WARNING_PRINTF("SendMessage LVM_SETITEMSTATE failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    }
}

#ifdef SORT_ENABLED
// state can be
// sortOrder - 0 neither, 1 ascending, 2 descending
void setListViewSortIcon(HWND listView, int col, int sortOrder)
{
    HWND headerHwnd;
    const int bufLen = 256;
    char headerText[bufLen];
    HD_ITEM item;
    int numColumns, curCol;

    headerHwnd = ListView_GetHeader(listView);
    numColumns = Header_GetItemCount(headerHwnd);

    for (curCol = 0; curCol < numColumns; curCol++) {
        item.mask = HDI_FORMAT | HDI_TEXT;
        item.pszText = headerText;
        item.cchTextMax = bufLen - 1;
        SendMessage(headerHwnd, HDM_GETITEM, curCol, (LPARAM)&item);

        if ((sortOrder != 0) && (curCol == col)) {
            switch (sortOrder) {
                case 1:
                    item.fmt &= !HDF_SORTUP;
                    item.fmt |= HDF_SORTDOWN;
                    break;
                case 2:
                    item.fmt &= !HDF_SORTDOWN;
                    item.fmt |= HDF_SORTUP;
                    break;
            }
        } else {
            item.fmt &= !HDF_SORTUP & !HDF_SORTDOWN;
        }
        item.fmt |= HDF_STRING;
        item.mask = HDI_FORMAT | HDI_TEXT;
        SendMessage(headerHwnd, HDM_SETITEM, curCol, (LPARAM)&item);
    }
}
#endif

void spySelectWindowAtPoint(const POINT & point)
{
    DEBUG_PRINTF("Mouse clicked at: %d, %d\n", point.x, point.y);

    HWND hwnd = WindowFromPoint(point);
    if (!hwnd) {
        DEBUG_PRINTF("No window found\n");
    } else {
        DEBUG_PRINTF("Spy mode: hwnd %#x\n", hwnd);

        HWND rootHwnd = GetAncestor(hwnd, GA_ROOT);
        if (!rootHwnd) {
            WARNING_PRINTF("Failed to get root hwnd, falling back to original\n");
            rootHwnd = hwnd;
        }

        CHAR executableFullPath[MAX_PATH] = {};
        DWORD processID;
        if (!GetWindowThreadProcessId(rootHwnd, &processID)) {
            WARNING_PRINTF("GetWindowThreadProcessId() failed: %s\n", StringUtility::lastErrorString().c_str());
        } else {
            HANDLE hproc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
            if (!hproc) {
                WARNING_PRINTF("OpenProcess() failed: %s\n", StringUtility::lastErrorString().c_str());
            } else {
                if (!GetModuleFileNameExA((HMODULE)hproc, nullptr, executableFullPath, MAX_PATH)) {
                    WARNING_PRINTF("GetModuleFileNameA() failed: %s\n", StringUtility::lastErrorString().c_str());
                }
                CloseHandle(hproc);
            }
        }
        DEBUG_PRINTF("Executable full path: '%s'\n", executableFullPath);

        std::string className = getWindowClassName(rootHwnd);
        DEBUG_PRINTF("Class name: '%s'\n", className.c_str());

        std::string title = getWindowText(rootHwnd);
        DEBUG_PRINTF("Title: '%s'\n", title.c_str());

        SetDlgItemTextA(spuModeHwnd_, IDC_AUTO_TRAY_EDIT_EXECUTABLE, executableFullPath);
        SetDlgItemTextA(spuModeHwnd_, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, className.c_str());
        SetDlgItemTextA(spuModeHwnd_, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, title.c_str());

        ShowWindow(spuModeHwnd_, SW_SHOW);
        SetForegroundWindow(spuModeHwnd_);

        spuModeHwnd_ = nullptr;
        spyMode_ = false;
    }
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if ((nCode == HC_ACTION) && (wParam == WM_LBUTTONDOWN)) {
        UnhookWindowsHookEx(mouseHhook_);

        LPMSLLHOOKSTRUCT mouseInfo = (LPMSLLHOOKSTRUCT)lParam;
        spySelectWindowAtPoint(mouseInfo->pt);
    }

    return CallNextHookEx(mouseHhook_, nCode, wParam, lParam);
}

std::string getDialogItemText(HWND dialogHwnd, int id)
{
    std::string text;
    int textLength = GetWindowTextLengthA(GetDlgItem(dialogHwnd, id));
    if (textLength > 0) {
        text.resize(textLength + 1);
        if (!GetDlgItemTextA(dialogHwnd, id, &text[0], (int)text.size())) {
            WARNING_PRINTF("GetDlgItemText failed: %s\n", StringUtility::lastErrorString().c_str());
            text.clear();
        } else {
            text.resize(textLength); // remove nul terminator
        }
    }
    return text;
}

std::string getListViewItemText(HWND listViewHwnd, int item, int subItem)
{
    std::string text;

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_TEXT;
    listViewItem.iItem = item;
    listViewItem.iSubItem = subItem;

    LRESULT res;
    do {
        text.resize(std::max<size_t>(256, text.size() * 2));
        listViewItem.pszText = &text[0];
        listViewItem.cchTextMax = (int)text.size();
        res = SendMessageA(listViewHwnd, LVM_GETITEMTEXTA, item, (LPARAM)&listViewItem);
        if (res == -1) {
            WARNING_PRINTF("SendMessage LVM_GETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
            return std::string();
        }
    } while (res >= (int)text.size() - 1);

    text.resize(res); // don't include nul terminator

    return text;
}

} // anonymous namespace
