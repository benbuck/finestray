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
#include "HandleWrapper.h"
#include "Helpers.h"
#include "Hotkey.h"
#include "Log.h"
#include "Resource.h"
#include "StringUtility.h"
#include "WindowInfo.h"

// Windows
#include <CommCtrl.h>
#include <Psapi.h>
#include <Windows.h>
#include <shellapi.h>

// Standard library
#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

// #define SORT_ENABLED

namespace
{

enum class AutoTrayListViewColumn : unsigned int
{
    WindowClass,
    Executable,
    WindowTitle,
    TrayEvent,
    Count
};

INT_PTR settingsDialogFunc(HWND dialogHwnd, UINT message, WPARAM wParam, LPARAM lParam);
void autoTrayListViewInit(HWND dialogHwnd);
std::vector<Settings::AutoTray> autoTrayListViewGetItems(HWND dialogHwnd);
void autoTrayListViewNotify(HWND dialogHwnd, const NMHDR * nmhdr);
#ifdef SORT_ENABLED
int autoTrayListViewCompare(LPARAM, LPARAM, LPARAM);
#endif
void autoTrayListViewItemAdd(HWND dialogHwnd);
void autoTrayListViewItemUpdate(HWND dialogHwnd, int item);
void autoTrayListViewItemDelete(HWND dialogHwnd, int item);
void autoTrayListViewItemEdit(HWND dialogHwnd, int item);
void autoTrayListViewUpdateButtons(HWND dialogHwnd);
void autoTrayListViewUpdateSelected(HWND dialogHwnd);
void spyBegin(HWND dialogHwnd);
void spyEnd(HWND dialogHwnd);
void spyEnableIcon(HWND dialogHwnd);
void spyDisableIcon(HWND dialogHwnd);
void spyUpdate(HWND dialogHwnd);
std::string getDialogItemText(HWND dialogHwnd, int id);
std::string getListViewItemText(HWND listViewHwnd, int item, int subItem);
TrayEvent resourceStringToTrayEvent(const std::string & str);
std::string trayEventToResourceString(TrayEvent trayEvent);

Settings settings_;
SettingsDialog::CompletionCallback completionCallback_;
HWND autoTrayListViewHwnd_;
int autoTrayListViewActiveItem_;
#ifdef SORT_ENABLED
bool autoTrayListViewSortAscending_;
int autoTrayListViewSortColumn_;
#endif
bool spyMode_;

} // anonymous namespace

namespace SettingsDialog
{
HWND create(HWND hwnd, const Settings & settings, CompletionCallback completionCallback)
{
    settings_ = settings;
    completionCallback_ = completionCallback;

    HINSTANCE hinstance = getInstance();
    HWND dialogHwnd = CreateDialogA(hinstance, MAKEINTRESOURCEA(IDD_DIALOG_SETTINGS), hwnd, settingsDialogFunc);

    // return value intentionally ignored, ShowWindow returns previous visibility
    ShowWindow(dialogHwnd, SW_SHOW);

    // return value intentionally ignored, SetForegroundWindow returns whether brought to foreground
    SetForegroundWindow(dialogHwnd);

    return dialogHwnd;
}

} // namespace SettingsDialog

namespace
{

INT_PTR settingsDialogFunc(HWND dialogHwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // DEBUG_PRINTF("wnd %#x, message %#x, wparam %#x, lparam %#x\n", dialogHwnd, message, wParam, lParam);

    if (spyMode_) {
        switch (message) {
            case WM_CAPTURECHANGED:
            case WM_LBUTTONUP: {
                spyUpdate(dialogHwnd);
                spyEnd(dialogHwnd);
                break;
            }

            case WM_MOUSEMOVE: {
                spyUpdate(dialogHwnd);
                break;
            }

            case WM_SETCURSOR: {
                HCURSOR hCursor = LoadCursor(nullptr, IDC_CROSS);
                if (!hCursor) {
                    WARNING_PRINTF("LoadCursor failed: %s\n", StringUtility::lastErrorString().c_str());
                } else {
                    if (!SetCursor(hCursor)) {
                        WARNING_PRINTF("SetCursor failed: %s\n", StringUtility::lastErrorString().c_str());
                    }
                }
                break;
            }

            default: {
                // DEBUG_PRINTF("spyMode message %#x, wparam %#x, lparam %#x\n", message, wParam, lParam);
                break;
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

            if (!CheckDlgButton(
                    dialogHwnd,
                    IDC_SHOW_WINDOWS_IN_MENU,
                    settings_.showWindowsInMenu_ ? BST_CHECKED : BST_UNCHECKED)) {
                WARNING_PRINTF("CheckDlgButton failed: %s\n", StringUtility::lastErrorString().c_str());
            }

            if (!CheckDlgButton(dialogHwnd, IDC_LOG_TO_FILE, settings_.logToFile_ ? BST_CHECKED : BST_UNCHECKED)) {
                WARNING_PRINTF("CheckDlgButton failed: %s\n", StringUtility::lastErrorString().c_str());
            }

            int checkButtonId = IDC_MINIMIZE_PLACEMENT_TRAY_AND_MENU;
            switch (settings_.minimizePlacement_) {
                case MinimizePlacement::Tray: checkButtonId = IDC_MINIMIZE_PLACEMENT_TRAY; break;
                case MinimizePlacement::Menu: checkButtonId = IDC_MINIMIZE_PLACEMENT_MENU; break;
                case MinimizePlacement::TrayAndMenu: checkButtonId = IDC_MINIMIZE_PLACEMENT_TRAY_AND_MENU; break;
                case MinimizePlacement::None:
                default: {
                    WARNING_PRINTF("bad minimize placement %d\n", settings_.minimizePlacement_);
                    break;
                }
            }

            if (!CheckRadioButton(
                    dialogHwnd,
                    IDC_MINIMIZE_PLACEMENT_TRAY,
                    IDC_MINIMIZE_PLACEMENT_TRAY_AND_MENU,
                    checkButtonId)) {
                WARNING_PRINTF("CheckRadioButton failed: %s\n", StringUtility::lastErrorString().c_str());
            }

            if (!SetDlgItemTextA(dialogHwnd, IDC_HOTKEY_MINIMIZE, settings_.hotkeyMinimize_.c_str())) {
                WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_HOTKEY_MINIMIZE_ALL, settings_.hotkeyMinimizeAll_.c_str())) {
                WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_HOTKEY_RESTORE, settings_.hotkeyRestore_.c_str())) {
                WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_HOTKEY_RESTORE_ALL, settings_.hotkeyRestoreAll_.c_str())) {
                WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_HOTKEY_MENU, settings_.hotkeyMenu_.c_str())) {
                WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_MODIFIER_OVERRIDE, settings_.modifiersOverride_.c_str())) {
                WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_POLL_INTERVAL, std::to_string(settings_.pollInterval_).c_str())) {
                WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, "")) {
                WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, "")) {
                WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }
            if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, "")) {
                WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
            }

            if (!CheckRadioButton(
                    dialogHwnd,
                    IDC_AUTO_TRAY_EVENT_OPEN,
                    IDC_AUTO_TRAY_EVENT_OPEN_AND_MINIMIZE,
                    IDC_AUTO_TRAY_EVENT_MINIMIZE)) {
                WARNING_PRINTF("CheckRadioButton failed: %s\n", StringUtility::lastErrorString().c_str());
            }

            spyEnableIcon(dialogHwnd);

            autoTrayListViewInit(dialogHwnd);

            break;
        }

        case WM_NOTIFY: {
            const NMHDR * nmhdr = reinterpret_cast<const NMHDR *>(lParam);
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

                    case IDC_SHOW_WINDOWS_IN_MENU: {
                        settings_.showWindowsInMenu_ = IsDlgButtonChecked(dialogHwnd, IDC_SHOW_WINDOWS_IN_MENU) ==
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

                    case IDC_MINIMIZE_PLACEMENT_TRAY_AND_MENU: {
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
                        if (HIWORD(wParam) == STN_CLICKED) {
                            spyBegin(dialogHwnd);
                        }
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
                        settings_.hotkeyMinimizeAll_ = getDialogItemText(dialogHwnd, IDC_HOTKEY_MINIMIZE_ALL);
                        settings_.hotkeyRestore_ = getDialogItemText(dialogHwnd, IDC_HOTKEY_RESTORE);
                        settings_.hotkeyRestoreAll_ = getDialogItemText(dialogHwnd, IDC_HOTKEY_RESTORE_ALL);
                        settings_.hotkeyMenu_ = getDialogItemText(dialogHwnd, IDC_HOTKEY_MENU);
                        settings_.modifiersOverride_ = getDialogItemText(dialogHwnd, IDC_MODIFIER_OVERRIDE);
                        settings_.pollInterval_ = std::stoul(getDialogItemText(dialogHwnd, IDC_POLL_INTERVAL));
                        settings_.autoTrays_ = autoTrayListViewGetItems(dialogHwnd);

                        EndDialog(dialogHwnd, static_cast<INT_PTR>(wParam));

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

                        EndDialog(dialogHwnd, static_cast<INT_PTR>(wParam));

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

    const int columnWeights[static_cast<size_t>(AutoTrayListViewColumn::Count)] = { 100, 150, 75, 75 };
    const int totalColumnWeight = std::accumulate(std::begin(columnWeights), std::end(columnWeights), 0);

    HWND hWndHdr = reinterpret_cast<HWND>(SendMessage(autoTrayListViewHwnd_, LVM_GETHEADER, 0, 0));
    const int count = static_cast<int>(SendMessage(hWndHdr, HDM_GETITEMCOUNT, 0, 0L));
    if (count < static_cast<int>(AutoTrayListViewColumn::Count)) {
        constexpr unsigned int styles = LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_ONECLICKACTIVATE |
            LVS_EX_GRIDLINES;
        if (SendMessageA(autoTrayListViewHwnd_, LVM_SETEXTENDEDLISTVIEWSTYLE, styles, styles) == -1) {
            WARNING_PRINTF(
                "SendMessage LVM_SETEXTENDEDLISTVIEWSTYLE failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }

        RECT rect;
        GetWindowRect(autoTrayListViewHwnd_, &rect);
        const int width = rect.right - rect.left;

        LVCOLUMNA listViewColumn;
        memset(&listViewColumn, 0, sizeof(listViewColumn));
        listViewColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        listViewColumn.fmt = LVCFMT_LEFT;

        std::string str;

        listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowClass);
        listViewColumn.cx = (width * columnWeights[static_cast<unsigned int>(AutoTrayListViewColumn::WindowClass)]) /
            totalColumnWeight;
        str = getResourceString(IDS_COLUMN_WINDOW_CLASS);
        listViewColumn.pszText = const_cast<LPSTR>(str.c_str());
        if (SendMessageA(
                autoTrayListViewHwnd_,
                LVM_INSERTCOLUMNA,
                listViewColumn.iSubItem,
                reinterpret_cast<LPARAM>(&listViewColumn)) == -1) {
            WARNING_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::Executable);
        listViewColumn.cx = (width * columnWeights[static_cast<unsigned int>(AutoTrayListViewColumn::Executable)]) /
            totalColumnWeight;
        str = getResourceString(IDS_COLUMN_EXECUTABLE);
        listViewColumn.pszText = const_cast<LPSTR>(str.c_str());
        if (SendMessageA(
                autoTrayListViewHwnd_,
                LVM_INSERTCOLUMNA,
                listViewColumn.iSubItem,
                reinterpret_cast<LPARAM>(&listViewColumn)) == -1) {
            WARNING_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowTitle);
        listViewColumn.cx = (width * columnWeights[static_cast<unsigned int>(AutoTrayListViewColumn::WindowTitle)]) /
            totalColumnWeight;
        str = getResourceString(IDS_COLUMN_WINDOW_TITLE);
        listViewColumn.pszText = const_cast<LPSTR>(str.c_str());
        if (SendMessageA(
                autoTrayListViewHwnd_,
                LVM_INSERTCOLUMNA,
                listViewColumn.iSubItem,
                reinterpret_cast<LPARAM>(&listViewColumn)) == -1) {
            WARNING_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        listViewColumn.iSubItem = static_cast<int>(AutoTrayListViewColumn::TrayEvent);
        listViewColumn.cx = (width * columnWeights[static_cast<unsigned int>(AutoTrayListViewColumn::TrayEvent)]) /
            totalColumnWeight;
        str = getResourceString(IDS_COLUMN_TRAY_EVENT);
        listViewColumn.pszText = const_cast<LPSTR>(str.c_str());
        if (SendMessageA(
                autoTrayListViewHwnd_,
                LVM_INSERTCOLUMNA,
                listViewColumn.iSubItem,
                reinterpret_cast<LPARAM>(&listViewColumn)) == -1) {
            WARNING_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    }

    if (!SendMessageA(autoTrayListViewHwnd_, LVM_DELETEALLITEMS, 0, 0)) {
        WARNING_PRINTF("SendMessage LVM_DELETEALLITEMS failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMCOUNT, static_cast<WPARAM>(settings_.autoTrays_.size()), 0) == -1) {
        WARNING_PRINTF("SendMessage LVM_SETITEMCOUNT failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));

    for (size_t a = 0; a < settings_.autoTrays_.size(); ++a) {
        listViewItem.iItem = static_cast<int>(a);
        listViewItem.lParam = static_cast<LPARAM>(a);
        listViewItem.mask = LVIF_TEXT | LVIF_PARAM;

        listViewItem.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowClass);
        listViewItem.pszText = const_cast<LPSTR>(settings_.autoTrays_.at(a).windowClass_.c_str());
        if (SendMessageA(autoTrayListViewHwnd_, LVM_INSERTITEMA, 0, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
            WARNING_PRINTF("SendMessage LVM_INSERTITEMA failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        listViewItem.iSubItem = static_cast<int>(AutoTrayListViewColumn::Executable);
        listViewItem.pszText = const_cast<LPSTR>(settings_.autoTrays_.at(a).executable_.c_str());
        if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, a, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
            WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        listViewItem.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowTitle);
        listViewItem.pszText = const_cast<LPSTR>(settings_.autoTrays_.at(a).windowTitle_.c_str());
        if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, a, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
            WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        const std::string str = trayEventToResourceString(settings_.autoTrays_.at(a).trayEvent_);
        listViewItem.iSubItem = static_cast<int>(AutoTrayListViewColumn::TrayEvent);
        listViewItem.pszText = const_cast<LPSTR>(str.c_str());
        if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, a, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
            WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
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

    const int itemCount = static_cast<int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    for (int item = 0; item < itemCount; ++item) {
        Settings::AutoTray autoTray;

        autoTray.windowClass_ =
            getListViewItemText(autoTrayListViewHwnd_, item, static_cast<int>(AutoTrayListViewColumn::WindowClass));
        autoTray.executable_ =
            getListViewItemText(autoTrayListViewHwnd_, item, static_cast<int>(AutoTrayListViewColumn::Executable));
        autoTray.windowTitle_ =
            getListViewItemText(autoTrayListViewHwnd_, item, static_cast<int>(AutoTrayListViewColumn::WindowTitle));

        const std::string trayEventStr =
            getListViewItemText(autoTrayListViewHwnd_, item, static_cast<int>(AutoTrayListViewColumn::TrayEvent));
        autoTray.trayEvent_ = resourceStringToTrayEvent(trayEventStr);

        if (!autoTray.executable_.empty() || !autoTray.windowClass_.empty() || !autoTray.windowTitle_.empty()) {
            autoTrays.push_back(autoTray);
        }
    }

    return autoTrays;
}

void autoTrayListViewNotify(HWND dialogHwnd, const NMHDR * nmhdr)
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
            const NMLISTVIEW * nmListView = reinterpret_cast<const NMLISTVIEW *>(nmhdr);
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
    DEBUG_PRINTF("Adding auto tray item\n");

    const int itemCount = static_cast<int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_TEXT | LVIF_PARAM;
    listViewItem.iItem = itemCount;
    listViewItem.lParam = itemCount;
    listViewItem.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowClass);
    if (SendMessageA(autoTrayListViewHwnd_, LVM_INSERTITEMA, 0, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
        WARNING_PRINTF("SendMessage LVM_INSERTITEMA failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    autoTrayListViewItemUpdate(dialogHwnd, itemCount);
    autoTrayListViewItemEdit(dialogHwnd, itemCount);

    autoTrayListViewActiveItem_ = itemCount;
    autoTrayListViewUpdateButtons(dialogHwnd);
    autoTrayListViewUpdateSelected(dialogHwnd);
}

void autoTrayListViewItemUpdate(HWND dialogHwnd, int item)
{
    DEBUG_PRINTF("Updating auto tray item %d\n", item);

    const int itemCount = static_cast<int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

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

    str = getWindowText(GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS));
    listViewItem.pszText = const_cast<LPSTR>(str.c_str());
    listViewItem.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowClass);
    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, item, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
        WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    str = getWindowText(GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE));
    listViewItem.pszText = const_cast<LPSTR>(str.c_str());
    listViewItem.iSubItem = static_cast<int>(AutoTrayListViewColumn::Executable);
    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, item, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
        WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    str = getWindowText(GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE));
    listViewItem.pszText = const_cast<LPSTR>(str.c_str());
    listViewItem.iSubItem = static_cast<int>(AutoTrayListViewColumn::WindowTitle);
    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, item, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
        WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    TrayEvent trayEvent = TrayEvent::None;
    if (IsDlgButtonChecked(dialogHwnd, IDC_AUTO_TRAY_EVENT_OPEN) == BST_CHECKED) {
        trayEvent = TrayEvent::Open;
    } else if (IsDlgButtonChecked(dialogHwnd, IDC_AUTO_TRAY_EVENT_MINIMIZE) == BST_CHECKED) {
        trayEvent = TrayEvent::Minimize;
    } else if (IsDlgButtonChecked(dialogHwnd, IDC_AUTO_TRAY_EVENT_OPEN_AND_MINIMIZE) == BST_CHECKED) {
        trayEvent = TrayEvent::OpenAndMinimize;
    } else {
        WARNING_PRINTF("No tray event selected");
    }

    str = trayEventToResourceString(trayEvent);
    listViewItem.pszText = const_cast<LPSTR>(str.c_str());
    listViewItem.iSubItem = static_cast<int>(AutoTrayListViewColumn::TrayEvent);
    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMTEXTA, item, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
        WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
    }
}

void autoTrayListViewItemDelete(HWND dialogHwnd, int item)
{
    DEBUG_PRINTF("Deleting auto tray item %d\n", item);

    int itemCount = static_cast<int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    if ((item < 0) || (item >= itemCount)) {
        WARNING_PRINTF("Item %d out of range\n", item);
        return;
    }

    if (!SendMessageA(autoTrayListViewHwnd_, LVM_DELETEITEM, item, 0)) {
        WARNING_PRINTF("SendMessage LVM_DELETEITEM failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    itemCount = static_cast<int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    if (autoTrayListViewActiveItem_ >= itemCount) {
        autoTrayListViewActiveItem_ = itemCount - 1;
    }
    autoTrayListViewUpdateButtons(dialogHwnd);
    autoTrayListViewUpdateSelected(dialogHwnd);
}

void autoTrayListViewItemEdit(HWND dialogHwnd, int item)
{
    DEBUG_PRINTF("Editing auto tray item %d\n", item);

    const int itemCount = static_cast<int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    if ((item < 0) || (item >= itemCount)) {
        WARNING_PRINTF("Item %d out of range\n", item);
        if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, "")) {
            WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
        }
        if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, "")) {
            WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
        }
        if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, "")) {
            WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
        }
        if (!CheckRadioButton(
                dialogHwnd,
                IDC_AUTO_TRAY_EVENT_OPEN,
                IDC_AUTO_TRAY_EVENT_OPEN_AND_MINIMIZE,
                IDC_AUTO_TRAY_EVENT_MINIMIZE)) {
            WARNING_PRINTF("CheckRadioButton failed: %s\n", StringUtility::lastErrorString().c_str());
        }
        autoTrayListViewActiveItem_ = -1;
    } else {
        const std::string windowClass =
            getListViewItemText(autoTrayListViewHwnd_, item, static_cast<int>(AutoTrayListViewColumn::WindowClass));
        if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, windowClass.c_str())) {
            WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        const std::string executable =
            getListViewItemText(autoTrayListViewHwnd_, item, static_cast<int>(AutoTrayListViewColumn::Executable));
        if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, executable.c_str())) {
            WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        const std::string windowTitle =
            getListViewItemText(autoTrayListViewHwnd_, item, static_cast<int>(AutoTrayListViewColumn::WindowTitle));
        if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, windowTitle.c_str())) {
            WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        const std::string trayEventStr =
            getListViewItemText(autoTrayListViewHwnd_, item, static_cast<int>(AutoTrayListViewColumn::TrayEvent));

        int checkButtonId = IDC_AUTO_TRAY_EVENT_MINIMIZE;
        if (trayEventStr == getResourceString(IDS_TRAY_EVENT_OPEN)) {
            checkButtonId = IDC_AUTO_TRAY_EVENT_OPEN;
        } else if (trayEventStr == getResourceString(IDS_TRAY_EVENT_MINIMIZE)) {
            checkButtonId = IDC_AUTO_TRAY_EVENT_MINIMIZE;
        } else if (trayEventStr == getResourceString(IDS_TRAY_EVENT_OPEN_AND_MINIMIZE)) {
            checkButtonId = IDC_AUTO_TRAY_EVENT_OPEN_AND_MINIMIZE;
        } else {
            WARNING_PRINTF("Unknown tray event %s\n", trayEventStr.c_str());
        }
        if (!CheckRadioButton(dialogHwnd, IDC_AUTO_TRAY_EVENT_OPEN, IDC_AUTO_TRAY_EVENT_OPEN_AND_MINIMIZE, checkButtonId)) {
            WARNING_PRINTF("CheckRadioButton failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        autoTrayListViewActiveItem_ = item;
    }

    autoTrayListViewUpdateButtons(dialogHwnd);
}

void autoTrayListViewUpdateButtons(HWND dialogHwnd)
{
    DEBUG_PRINTF("Updating buttons\n");

    const int itemCount = static_cast<int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    const bool hasActiveItem = (autoTrayListViewActiveItem_ >= 0) && (autoTrayListViewActiveItem_ < itemCount);
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
        if (SendMessageA(
                autoTrayListViewHwnd_,
                LVM_SETITEMSTATE,
                static_cast<WPARAM>(-1),
                reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
            WARNING_PRINTF("SendMessage LVM_SETITEMSTATE failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    } else {
        if (!SetFocus(autoTrayListViewHwnd_)) {
            WARNING_PRINTF("SetFocus failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        if (SendMessageA(autoTrayListViewHwnd_, LVM_ENSUREVISIBLE, autoTrayListViewActiveItem_, static_cast<LPARAM>(TRUE)) ==
            -1) {
            WARNING_PRINTF("SendMessage LVM_ENSUREVISIBLE failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        LVITEM listViewItem;
        memset(&listViewItem, 0, sizeof(listViewItem));
        listViewItem.mask = LVIF_STATE;
        listViewItem.state = LVIS_FOCUSED | LVIS_SELECTED;
        listViewItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        if (SendMessageA(
                autoTrayListViewHwnd_,
                LVM_SETITEMSTATE,
                autoTrayListViewActiveItem_,
                reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
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
        SendMessage(headerHwnd, HDM_GETITEM, curCol, reinterpret_cast<LPARAM>(&item));

        if ((sortOrder != 0) && (curCol == col)) {
            switch (sortOrder) {
                case 1: {
                    item.fmt &= ~HDF_SORTUP;
                    item.fmt |= HDF_SORTDOWN;
                    break;
                }
                case 2: {
                    item.fmt &= ~HDF_SORTDOWN;
                    item.fmt |= HDF_SORTUP;
                    break;
                }
                default: {
                    ERROR_PRINTF("Unknown sort order %d\n", sortOrder);
                    break;
                }
            }
        } else {
            item.fmt &= ~HDF_SORTUP;
            item.fmt &= ~HDF_SORTDOWN;
        }
        item.fmt |= HDF_STRING;
        item.mask = HDI_FORMAT | HDI_TEXT;
        SendMessage(headerHwnd, HDM_SETITEM, curCol, reinterpret_cast<LPARAM>(&item));
    }
}
#endif

void spyBegin(HWND dialogHwnd)
{
    DEBUG_PRINTF("Spy mode: beginning\n");

    spyMode_ = true;

    spyDisableIcon(dialogHwnd);

    if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, "")) {
        WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
    }
    if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, "")) {
        WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
    }
    if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, "")) {
        WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    SetCapture(dialogHwnd);
}

void spyEnd(HWND dialogHwnd)
{
    DEBUG_PRINTF("Spy mode: ended\n");
    if (!ReleaseCapture()) {
        WARNING_PRINTF("ReleaseCapture failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    spyEnableIcon(dialogHwnd);

    spyMode_ = false;
}

void spyEnableIcon(HWND dialogHwnd)
{
    HCURSOR hCrossCursor = LoadCursor(nullptr, IDC_CROSS);
    if (!hCrossCursor) {
        WARNING_PRINTF("LoadCursor failed: %s\n", StringUtility::lastErrorString().c_str());
    } else {
        // return value ignored, poorly defined
        SendDlgItemMessage(dialogHwnd, IDC_AUTO_TRAY_ITEM_SPY, STM_SETICON, reinterpret_cast<WPARAM>(hCrossCursor), 0);
    }
}

void spyDisableIcon(HWND dialogHwnd)
{
    // return value ignored, poorly defined
    SendDlgItemMessage(dialogHwnd, IDC_AUTO_TRAY_ITEM_SPY, STM_SETICON, 0, 0);

    HCURSOR hCursor = LoadCursor(nullptr, IDC_CROSS);
    if (!hCursor) {
        WARNING_PRINTF("LoadCursor failed: %s\n", StringUtility::lastErrorString().c_str());
    } else {
        if (!SetCursor(hCursor)) {
            WARNING_PRINTF("SetCursor failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    }
}

void spyUpdate(HWND dialogHwnd)
{
    POINT point;
    if (!GetCursorPos(&point)) {
        WARNING_PRINTF("GetCursorPos failed: %s\n", StringUtility::lastErrorString().c_str());
        return;
    }

    DEBUG_PRINTF("Spy mode: selecting window at: %d, %d\n", point.x, point.y);

    HWND hwnd = WindowFromPoint(point);
    if (!hwnd) {
        DEBUG_PRINTF("No window found\n");
        return;
    }

    HWND rootHwnd = GetAncestor(hwnd, GA_ROOT);
    if (!rootHwnd) {
        WARNING_PRINTF("Failed to get root hwnd, falling back to original\n");
        rootHwnd = hwnd;
    }

    if (rootHwnd == dialogHwnd) {
        DEBUG_PRINTF("Spy mode: own window, clearing\n");
        if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, "")) {
            WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
        }
        if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, "")) {
            WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
        }
        if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, "")) {
            WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
        }
        return;
    }

    DEBUG_PRINTF("Spy mode: root hwnd %#x\n", rootHwnd);

    const WindowInfo windowInfo(rootHwnd);
    DEBUG_PRINTF("Class name: '%s'\n", windowInfo.className().c_str());
    DEBUG_PRINTF("Executable full path: '%s'\n", windowInfo.executable().c_str());
    DEBUG_PRINTF("Title: '%s'\n", windowInfo.title().c_str());

    if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, windowInfo.className().c_str())) {
        WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
    }
    if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, windowInfo.executable().c_str())) {
        WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
    }
    if (!SetDlgItemTextA(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, windowInfo.title().c_str())) {
        WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
    }
}

std::string getDialogItemText(HWND dialogHwnd, int id)
{
    std::string text;
    const int textLength = GetWindowTextLengthA(GetDlgItem(dialogHwnd, id));
    if (textLength > 0) {
        text.resize(static_cast<size_t>(textLength) + 1);
        if (!GetDlgItemTextA(dialogHwnd, id, text.data(), static_cast<int>(text.size()))) {
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

    while (true) {
        text.resize(std::max<size_t>(256, text.size() * 2));
        listViewItem.pszText = text.data();
        listViewItem.cchTextMax = static_cast<int>(text.size());
        LRESULT const res = SendMessageA(listViewHwnd, LVM_GETITEMTEXTA, item, reinterpret_cast<LPARAM>(&listViewItem));
        if (res == -1) {
            WARNING_PRINTF("SendMessage LVM_GETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
            return {};
        }
        if ((static_cast<int>(text.size()) - 1) > res) {
            text.resize(res); // don't include nul terminator
            break;
        }
    }

    return text;
}

TrayEvent resourceStringToTrayEvent(const std::string & str)
{
    if (str == getResourceString(IDS_TRAY_EVENT_OPEN)) {
        return TrayEvent::Open;
    }

    if (str == getResourceString(IDS_TRAY_EVENT_MINIMIZE)) {
        return TrayEvent::Minimize;
    }

    if (str == getResourceString(IDS_TRAY_EVENT_OPEN_AND_MINIMIZE)) {
        return TrayEvent::OpenAndMinimize;
    }

    WARNING_PRINTF("Unknown tray event %s\n", str.c_str());
    return TrayEvent::None;
}

std::string trayEventToResourceString(TrayEvent trayEvent)
{
    switch (trayEvent) {
        case TrayEvent::Open: return getResourceString(IDS_TRAY_EVENT_OPEN);
        case TrayEvent::Minimize: return getResourceString(IDS_TRAY_EVENT_MINIMIZE);
        case TrayEvent::OpenAndMinimize: return getResourceString(IDS_TRAY_EVENT_OPEN_AND_MINIMIZE);
        default: {
            WARNING_PRINTF("Unknown tray event %d\n", (int)trayEvent);
            return "";
        }
    }
}

} // anonymous namespace
