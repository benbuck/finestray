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

// App
#include "SettingsDialog.h"
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

enum AutoTrayListViewColumn
{
    AutoTrayListViewColumn_WindowClass,
    AutoTrayListViewColumn_Executable,
    AutoTrayListViewColumn_WindowTitle,
    AutoTrayListViewColumn_TrayEvent,
    AutoTrayListViewColumn_MinimizePersistence,
    AutoTrayListViewColumn_Count
};

INT_PTR settingsDialogFunc(HWND dialogHwnd, UINT message, WPARAM wParam, LPARAM lParam);
void autoTrayListViewInit(HWND dialogHwnd);
std::vector<Settings::AutoTray> autoTrayListViewGetItems(HWND dialogHwnd);
void autoTrayListViewNotify(HWND dialogHwnd, const NMHDR * nmhdr);
#ifdef SORT_ENABLED
int autoTrayListViewCompare(LPARAM, LPARAM, LPARAM);
#endif
void autoTrayListViewItemAdd(HWND dialogHwnd);
void autoTrayListViewItemUpdate(HWND dialogHwnd, unsigned int item);
void autoTrayListViewItemDelete(HWND dialogHwnd, unsigned int item);
void autoTrayListViewItemEdit(HWND dialogHwnd, unsigned int item);
void autoTrayListViewUpdateButtons(HWND dialogHwnd) noexcept;
void autoTrayListViewUpdateSelected(HWND dialogHwnd);
void spyBegin(HWND dialogHwnd);
void spyEnd(HWND dialogHwnd);
void spyEnableIcon(HWND dialogHwnd);
void spyDisableIcon(HWND dialogHwnd);
void spyUpdate(HWND dialogHwnd);
std::string getDialogItemText(HWND dialogHwnd, int id);
std::string getListViewItemText(HWND listViewHwnd, unsigned int item, int subItem);
void setDlgItemTextSafe(HWND dialogHwnd, int id, const std::string & text);
void checkDlgButtonSafe(HWND dialogHwnd, int id, bool check);
void checkRadioButtonSafe(HWND dialogHwnd, int firstId, int lastId, int checkId);
void insertColumnSafe(HWND listViewHwnd, unsigned int columnIndex, int width, const char * text);
void insertItemSafe(HWND listViewHwnd, unsigned int item, int subItem, const char * text);
void setItemTextSafe(HWND listViewHwnd, unsigned int item, int subItem, const char * text);
void setItemStateSafe(HWND listViewHwnd, unsigned int item, UINT state, UINT stateMask);
TrayEvent resourceStringToTrayEvent(const std::string & str);
std::string trayEventToResourceString(TrayEvent trayEvent);
MinimizePersistence resourceStringToMinimizePersistence(const std::string & str);
std::string minimizePersistenceToResourceString(MinimizePersistence minimizePersistence);

Settings settings_;
SettingsDialog::CompletionCallback completionCallback_;
HWND autoTrayListViewHwnd_;
unsigned int autoTrayListViewActiveItem_;
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
            checkDlgButtonSafe(dialogHwnd, IDC_START_WITH_WINDOWS, settings_.startWithWindows_);
            checkDlgButtonSafe(dialogHwnd, IDC_LOG_TO_FILE, settings_.logToFile_);

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

            checkRadioButtonSafe(dialogHwnd, IDC_MINIMIZE_PLACEMENT_TRAY, IDC_MINIMIZE_PLACEMENT_TRAY_AND_MENU, checkButtonId);

            setDlgItemTextSafe(dialogHwnd, IDC_HOTKEY_MINIMIZE, settings_.hotkeyMinimize_);
            setDlgItemTextSafe(dialogHwnd, IDC_HOTKEY_MINIMIZE_ALL, settings_.hotkeyMinimizeAll_);
            setDlgItemTextSafe(dialogHwnd, IDC_HOTKEY_RESTORE, settings_.hotkeyRestore_);
            setDlgItemTextSafe(dialogHwnd, IDC_HOTKEY_RESTORE_ALL, settings_.hotkeyRestoreAll_);
            setDlgItemTextSafe(dialogHwnd, IDC_HOTKEY_MENU, settings_.hotkeyMenu_);
            setDlgItemTextSafe(dialogHwnd, IDC_MODIFIER_OVERRIDE, settings_.modifiersOverride_);
            setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, "");
            setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, "");
            setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, "");

            checkRadioButtonSafe(
                dialogHwnd,
                IDC_AUTO_TRAY_EVENT_OPEN,
                IDC_AUTO_TRAY_EVENT_OPEN_AND_MINIMIZE,
                IDC_AUTO_TRAY_EVENT_MINIMIZE);

            checkRadioButtonSafe(
                dialogHwnd,
                IDC_AUTO_TRAY_PERSIST_NEVER,
                IDC_AUTO_TRAY_PERSIST_ALWAYS,
                IDC_AUTO_TRAY_PERSIST_NEVER);

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
                        settings_.autoTrays_ = autoTrayListViewGetItems(dialogHwnd);

                        EndDialog(dialogHwnd, narrow_cast<INT_PTR>(wParam));

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

                        EndDialog(dialogHwnd, narrow_cast<INT_PTR>(wParam));

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

    const int columnWeights[AutoTrayListViewColumn_Count] = { 75, 150, 100, 75, 50 };
    const int totalColumnWeight = std::accumulate(std::begin(columnWeights), std::end(columnWeights), 0);

    HWND hWndHdr = reinterpret_cast<HWND>(SendMessage(autoTrayListViewHwnd_, LVM_GETHEADER, 0, 0));
    const unsigned int count = narrow_cast<unsigned int>(SendMessage(hWndHdr, HDM_GETITEMCOUNT, 0, 0L));
    if (count < AutoTrayListViewColumn_Count) {
        constexpr unsigned int styles = LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_ONECLICKACTIVATE |
            LVS_EX_GRIDLINES;
        if (SendMessageA(autoTrayListViewHwnd_, LVM_SETEXTENDEDLISTVIEWSTYLE, styles, styles) == -1) {
            WARNING_PRINTF(
                "SendMessage LVM_SETEXTENDEDLISTVIEWSTYLE failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }

        RECT rect;
        GetWindowRect(autoTrayListViewHwnd_, &rect);
        const int listViewWidth = rect.right - rect.left;

        int width = (listViewWidth * columnWeights[AutoTrayListViewColumn_WindowClass]) / totalColumnWeight;
        insertColumnSafe(
            autoTrayListViewHwnd_,
            AutoTrayListViewColumn_WindowClass,
            width,
            getResourceString(IDS_COLUMN_WINDOW_CLASS).c_str());

        width = (listViewWidth * columnWeights[AutoTrayListViewColumn_Executable]) / totalColumnWeight;
        insertColumnSafe(
            autoTrayListViewHwnd_,
            AutoTrayListViewColumn_Executable,
            width,
            getResourceString(IDS_COLUMN_EXECUTABLE).c_str());

        width = (listViewWidth * columnWeights[AutoTrayListViewColumn_WindowTitle]) / totalColumnWeight;
        insertColumnSafe(
            autoTrayListViewHwnd_,
            AutoTrayListViewColumn_WindowTitle,
            width,
            getResourceString(IDS_COLUMN_WINDOW_TITLE).c_str());

        width = (listViewWidth * columnWeights[AutoTrayListViewColumn_TrayEvent]) / totalColumnWeight;
        insertColumnSafe(
            autoTrayListViewHwnd_,
            AutoTrayListViewColumn_TrayEvent,
            width,
            getResourceString(IDS_COLUMN_TRAY_EVENT).c_str());

        width = (listViewWidth * columnWeights[AutoTrayListViewColumn_MinimizePersistence]) / totalColumnWeight;
        insertColumnSafe(
            autoTrayListViewHwnd_,
            AutoTrayListViewColumn_MinimizePersistence,
            width,
            getResourceString(IDS_COLUMN_MINIMIZE_PERSISTENCE).c_str());
    }

    if (!SendMessageA(autoTrayListViewHwnd_, LVM_DELETEALLITEMS, 0, 0)) {
        WARNING_PRINTF("SendMessage LVM_DELETEALLITEMS failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    if (SendMessageA(autoTrayListViewHwnd_, LVM_SETITEMCOUNT, narrow_cast<WPARAM>(settings_.autoTrays_.size()), 0) == -1) {
        WARNING_PRINTF("SendMessage LVM_SETITEMCOUNT failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));

    for (size_t a = 0; a < settings_.autoTrays_.size(); ++a) {
        listViewItem.iItem = narrow_cast<int>(a);
        listViewItem.lParam = narrow_cast<LPARAM>(a);
        listViewItem.mask = LVIF_TEXT | LVIF_PARAM;

        insertItemSafe(
            autoTrayListViewHwnd_,
            narrow_cast<unsigned int>(a),
            AutoTrayListViewColumn_WindowClass,
            settings_.autoTrays_.at(a).windowClass_.c_str());

        setItemTextSafe(
            autoTrayListViewHwnd_,
            narrow_cast<unsigned int>(a),
            AutoTrayListViewColumn_Executable,
            settings_.autoTrays_.at(a).executable_.c_str());

        setItemTextSafe(
            autoTrayListViewHwnd_,
            narrow_cast<unsigned int>(a),
            AutoTrayListViewColumn_WindowTitle,
            settings_.autoTrays_.at(a).windowTitle_.c_str());

        setItemTextSafe(
            autoTrayListViewHwnd_,
            narrow_cast<unsigned int>(a),
            AutoTrayListViewColumn_TrayEvent,
            trayEventToResourceString(settings_.autoTrays_.at(a).trayEvent_).c_str());

        setItemTextSafe(
            autoTrayListViewHwnd_,
            narrow_cast<unsigned int>(a),
            AutoTrayListViewColumn_MinimizePersistence,
            minimizePersistenceToResourceString(settings_.autoTrays_.at(a).minimizePersistence_).c_str());
    }

    autoTrayListViewActiveItem_ = ~0U;
#ifdef SORT_ENABLED
    autoTrayListViewSortAscending_ = true;
    autoTrayListViewSortColumn_ = 0;
#endif

    autoTrayListViewUpdateButtons(dialogHwnd);
}

std::vector<Settings::AutoTray> autoTrayListViewGetItems(HWND /* dialogHwnd */)
{
    std::vector<Settings::AutoTray> autoTrays;

    const unsigned int itemCount = narrow_cast<unsigned int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    for (unsigned int item = 0; item < itemCount; ++item) {
        Settings::AutoTray autoTray;

        autoTray.windowClass_ = getListViewItemText(autoTrayListViewHwnd_, item, AutoTrayListViewColumn_WindowClass);
        autoTray.executable_ = getListViewItemText(autoTrayListViewHwnd_, item, AutoTrayListViewColumn_Executable);
        autoTray.windowTitle_ = getListViewItemText(autoTrayListViewHwnd_, item, AutoTrayListViewColumn_WindowTitle);

        const std::string trayEventStr =
            getListViewItemText(autoTrayListViewHwnd_, item, AutoTrayListViewColumn_TrayEvent);
        autoTray.trayEvent_ = resourceStringToTrayEvent(trayEventStr);

        const std::string minimizePersistenceStr =
            getListViewItemText(autoTrayListViewHwnd_, item, AutoTrayListViewColumn_MinimizePersistence);
        autoTray.minimizePersistence_ = resourceStringToMinimizePersistence(minimizePersistenceStr);

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
                    autoTrayListViewItemEdit(dialogHwnd, narrow_cast<unsigned int>(nmListView->iItem));
                } else {
                    autoTrayListViewItemEdit(dialogHwnd, ~0U);
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

    const unsigned int itemCount = narrow_cast<unsigned int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    // create the row so that the update can fill it in correctly
    insertItemSafe(
        autoTrayListViewHwnd_,
        itemCount,
        AutoTrayListViewColumn_WindowClass,
        getDialogItemText(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS).c_str());

    autoTrayListViewItemUpdate(dialogHwnd, itemCount);
    autoTrayListViewItemEdit(dialogHwnd, itemCount);

    autoTrayListViewActiveItem_ = itemCount;
    autoTrayListViewUpdateButtons(dialogHwnd);
    autoTrayListViewUpdateSelected(dialogHwnd);
}

void autoTrayListViewItemUpdate(HWND dialogHwnd, unsigned int item)
{
    DEBUG_PRINTF("Updating auto tray item %d\n", item);

    const int unsigned itemCount = narrow_cast<unsigned int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    if (item >= itemCount) {
        WARNING_PRINTF("Item %d out of range\n", item);
        return;
    }

    setItemTextSafe(
        autoTrayListViewHwnd_,
        item,
        AutoTrayListViewColumn_WindowClass,
        getDialogItemText(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS).c_str());

    setItemTextSafe(
        autoTrayListViewHwnd_,
        item,
        AutoTrayListViewColumn_Executable,
        getDialogItemText(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE).c_str());

    setItemTextSafe(
        autoTrayListViewHwnd_,
        item,
        AutoTrayListViewColumn_WindowTitle,
        getDialogItemText(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE).c_str());

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

    setItemTextSafe(
        autoTrayListViewHwnd_,
        item,
        AutoTrayListViewColumn_TrayEvent,
        trayEventToResourceString(trayEvent).c_str());

    MinimizePersistence minimizePersistence = MinimizePersistence::None;
    if (IsDlgButtonChecked(dialogHwnd, IDC_AUTO_TRAY_PERSIST_NEVER) == BST_CHECKED) {
        minimizePersistence = MinimizePersistence::Never;
    } else if (IsDlgButtonChecked(dialogHwnd, IDC_AUTO_TRAY_PERSIST_ALWAYS) == BST_CHECKED) {
        minimizePersistence = MinimizePersistence::Always;
    } else {
        WARNING_PRINTF("No minimize persistence selected");
    }

    setItemTextSafe(
        autoTrayListViewHwnd_,
        item,
        AutoTrayListViewColumn_MinimizePersistence,
        minimizePersistenceToResourceString(minimizePersistence).c_str());
}

void autoTrayListViewItemDelete(HWND dialogHwnd, unsigned int item)
{
    DEBUG_PRINTF("Deleting auto tray item %d\n", item);

    unsigned int itemCount = narrow_cast<unsigned int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    if (item >= itemCount) {
        WARNING_PRINTF("Item %d out of range\n", item);
        return;
    }

    if (!SendMessageA(autoTrayListViewHwnd_, LVM_DELETEITEM, item, 0)) {
        WARNING_PRINTF("SendMessage LVM_DELETEITEM failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    itemCount = narrow_cast<unsigned int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    if (autoTrayListViewActiveItem_ >= itemCount) {
        autoTrayListViewActiveItem_ = itemCount - 1;
    }
    autoTrayListViewUpdateButtons(dialogHwnd);
    autoTrayListViewUpdateSelected(dialogHwnd);
}

void autoTrayListViewItemEdit(HWND dialogHwnd, unsigned int item)
{
    DEBUG_PRINTF("Editing auto tray item %d\n", item);

    const unsigned int itemCount = narrow_cast<unsigned int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    if (item >= itemCount) {
        WARNING_PRINTF("Item %d out of range\n", item);
        setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, "");
        setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, "");
        setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, "");
        checkRadioButtonSafe(
            dialogHwnd,
            IDC_AUTO_TRAY_EVENT_OPEN,
            IDC_AUTO_TRAY_EVENT_OPEN_AND_MINIMIZE,
            IDC_AUTO_TRAY_EVENT_MINIMIZE);
        autoTrayListViewActiveItem_ = ~0U;
    } else {
        setDlgItemTextSafe(
            dialogHwnd,
            IDC_AUTO_TRAY_EDIT_WINDOWCLASS,
            getListViewItemText(autoTrayListViewHwnd_, item, AutoTrayListViewColumn_WindowClass));
        setDlgItemTextSafe(
            dialogHwnd,
            IDC_AUTO_TRAY_EDIT_EXECUTABLE,
            getListViewItemText(autoTrayListViewHwnd_, item, AutoTrayListViewColumn_Executable));
        setDlgItemTextSafe(
            dialogHwnd,
            IDC_AUTO_TRAY_EDIT_WINDOWTITLE,
            getListViewItemText(autoTrayListViewHwnd_, item, AutoTrayListViewColumn_WindowTitle));

        const std::string trayEventStr =
            getListViewItemText(autoTrayListViewHwnd_, item, AutoTrayListViewColumn_TrayEvent);
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
        checkRadioButtonSafe(dialogHwnd, IDC_AUTO_TRAY_EVENT_OPEN, IDC_AUTO_TRAY_EVENT_OPEN_AND_MINIMIZE, checkButtonId);

        const std::string minimizePersistenceStr =
            getListViewItemText(autoTrayListViewHwnd_, item, AutoTrayListViewColumn_MinimizePersistence);
        checkButtonId = IDC_AUTO_TRAY_PERSIST_NEVER;
        if (minimizePersistenceStr == getResourceString(IDS_MINIMIZE_PERSISTENCE_NEVER)) {
            checkButtonId = IDC_AUTO_TRAY_PERSIST_NEVER;
        } else if (minimizePersistenceStr == getResourceString(IDS_MINIMIZE_PERSISTENCE_ALWAYS)) {
            checkButtonId = IDC_AUTO_TRAY_PERSIST_ALWAYS;
        } else {
            WARNING_PRINTF("Unknown minimize persistence %s\n", minimizePersistenceStr.c_str());
        }
        checkRadioButtonSafe(dialogHwnd, IDC_AUTO_TRAY_PERSIST_NEVER, IDC_AUTO_TRAY_PERSIST_ALWAYS, checkButtonId);

        autoTrayListViewActiveItem_ = item;
    }

    autoTrayListViewUpdateButtons(dialogHwnd);
}

void autoTrayListViewUpdateButtons(HWND dialogHwnd) noexcept
{
    DEBUG_PRINTF("Updating buttons\n");

    const unsigned int itemCount = narrow_cast<unsigned int>(SendMessageA(autoTrayListViewHwnd_, LVM_GETITEMCOUNT, 0, 0));

    const bool hasActiveItem = (autoTrayListViewActiveItem_ < itemCount);
    EnableWindow(GetDlgItem(dialogHwnd, IDC_AUTO_TRAY_ITEM_DELETE), hasActiveItem);
}

void autoTrayListViewUpdateSelected(HWND /*dialogHwnd*/)
{
    DEBUG_PRINTF("Updating selected %d\n", autoTrayListViewActiveItem_);

    if (autoTrayListViewActiveItem_ == ~0U) {
        setItemStateSafe(autoTrayListViewHwnd_, ~0U, 0, LVIS_FOCUSED | LVIS_SELECTED);
    } else {
        if (!SetFocus(autoTrayListViewHwnd_)) {
            WARNING_PRINTF("SetFocus failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        if (SendMessageA(autoTrayListViewHwnd_, LVM_ENSUREVISIBLE, autoTrayListViewActiveItem_, static_cast<LPARAM>(TRUE)) ==
            -1) {
            WARNING_PRINTF("SendMessage LVM_ENSUREVISIBLE failed: %s\n", StringUtility::lastErrorString().c_str());
        }

        setItemStateSafe(
            autoTrayListViewHwnd_,
            autoTrayListViewActiveItem_,
            LVIS_FOCUSED | LVIS_SELECTED,
            LVIS_FOCUSED | LVIS_SELECTED);
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

    setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, "");
    setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, "");
    setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, "");

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
        setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, "");
        setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, "");
        setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, "");
        return;
    }

    DEBUG_PRINTF("Spy mode: root hwnd %#x\n", rootHwnd);

    const WindowInfo windowInfo(rootHwnd);
    DEBUG_PRINTF("Class name: '%s'\n", windowInfo.className().c_str());
    DEBUG_PRINTF("Executable full path: '%s'\n", windowInfo.executable().c_str());
    DEBUG_PRINTF("Title: '%s'\n", windowInfo.title().c_str());

    setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWCLASS, windowInfo.className());
    setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_EXECUTABLE, windowInfo.executable());
    setDlgItemTextSafe(dialogHwnd, IDC_AUTO_TRAY_EDIT_WINDOWTITLE, windowInfo.title());
}

std::string getDialogItemText(HWND dialogHwnd, int id)
{
    std::string text;
    const int textLength = GetWindowTextLengthA(GetDlgItem(dialogHwnd, id));
    if (textLength > 0) {
        text.resize(static_cast<size_t>(textLength) + 1);
        if (!GetDlgItemTextA(dialogHwnd, id, text.data(), narrow_cast<int>(text.size()))) {
            WARNING_PRINTF("GetDlgItemText failed: %s\n", StringUtility::lastErrorString().c_str());
            text.clear();
        } else {
            text.resize(narrow_cast<size_t>(textLength)); // remove nul terminator
        }
    }
    return text;
}

std::string getListViewItemText(HWND listViewHwnd, unsigned int item, int subItem)
{
    std::string text;

    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_TEXT;
    listViewItem.iItem = narrow_cast<int>(item);
    listViewItem.iSubItem = subItem;

    while (true) {
        text.resize(std::max<size_t>(256, text.size() * 2));
        listViewItem.pszText = text.data();
        listViewItem.cchTextMax = narrow_cast<int>(text.size());
        LRESULT const res = SendMessageA(listViewHwnd, LVM_GETITEMTEXTA, item, reinterpret_cast<LPARAM>(&listViewItem));
        if (res == -1) {
            WARNING_PRINTF("SendMessage LVM_GETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
            return {};
        }
        if (narrow_cast<int>(text.size() - 1) > res) {
            text.resize(narrow_cast<size_t>(res)); // don't include nul terminator
            break;
        }
    }

    return text;
}

void setDlgItemTextSafe(HWND dialogHwnd, int id, const std::string & text)
{
    if (!SetDlgItemTextA(dialogHwnd, id, text.c_str())) {
        WARNING_PRINTF("SetDlgItemTextA failed: %s\n", StringUtility::lastErrorString().c_str());
    }
}

void checkDlgButtonSafe(HWND dialogHwnd, int id, bool check)
{
    if (!CheckDlgButton(dialogHwnd, id, check ? BST_CHECKED : BST_UNCHECKED)) {
        WARNING_PRINTF("CheckDlgButton failed: %s\n", StringUtility::lastErrorString().c_str());
    }
}

void checkRadioButtonSafe(HWND dialogHwnd, int firstId, int lastId, int checkId)
{
    if (!CheckRadioButton(dialogHwnd, firstId, lastId, checkId)) {
        WARNING_PRINTF("CheckRadioButton failed: %s\n", StringUtility::lastErrorString().c_str());
    }
}

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 26492) // Don't use const_cast to cast away const or volatile
#endif

void insertColumnSafe(HWND listViewHwnd, unsigned int columnIndex, int width, const char * text)
{
    LVCOLUMNA listViewColumn;
    memset(&listViewColumn, 0, sizeof(listViewColumn));
    listViewColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    listViewColumn.fmt = LVCFMT_LEFT;
    listViewColumn.iSubItem = narrow_cast<int>(columnIndex);
    listViewColumn.cx = width;
    listViewColumn.pszText = const_cast<LPSTR>(text);
    if (SendMessageA(listViewHwnd, LVM_INSERTCOLUMNA, columnIndex, reinterpret_cast<LPARAM>(&listViewColumn)) == -1) {
        WARNING_PRINTF("SendMessage LVM_INSERTCOLUMNA failed: %s\n", StringUtility::lastErrorString().c_str());
    }
}

void insertItemSafe(HWND listViewHwnd, unsigned int item, int subItem, const char * text)
{
    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.iItem = narrow_cast<int>(item);
    listViewItem.lParam = static_cast<LPARAM>(item);
    listViewItem.iSubItem = subItem;
    listViewItem.pszText = const_cast<LPSTR>(text);
    listViewItem.mask = LVIF_TEXT | LVIF_PARAM;

    if (SendMessageA(listViewHwnd, LVM_INSERTITEMA, 0, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
        WARNING_PRINTF("SendMessage LVM_INSERTITEMA failed: %s\n", StringUtility::lastErrorString().c_str());
    }
}

void setItemTextSafe(HWND listViewHwnd, unsigned int item, int subItem, const char * text)
{
    LVITEMA listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.iItem = narrow_cast<int>(item);
    listViewItem.lParam = static_cast<LPARAM>(item);
    listViewItem.iSubItem = subItem;
    listViewItem.pszText = const_cast<LPSTR>(text);
    listViewItem.mask = LVIF_TEXT | LVIF_PARAM;

    if (SendMessageA(listViewHwnd, LVM_SETITEMTEXTA, item, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
        WARNING_PRINTF("SendMessage LVM_SETITEMTEXTA failed: %s\n", StringUtility::lastErrorString().c_str());
    }
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

void setItemStateSafe(HWND listViewHwnd, unsigned int item, UINT state, UINT stateMask)
{
    LVITEM listViewItem;
    memset(&listViewItem, 0, sizeof(listViewItem));
    listViewItem.mask = LVIF_STATE;
    listViewItem.state = state;
    listViewItem.stateMask = stateMask;
    if (SendMessageA(listViewHwnd, LVM_SETITEMSTATE, item, reinterpret_cast<LPARAM>(&listViewItem)) == -1) {
        WARNING_PRINTF("SendMessage LVM_SETITEMSTATE failed: %s\n", StringUtility::lastErrorString().c_str());
    }
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
        case TrayEvent::None:
        default: {
            WARNING_PRINTF("Unknown tray event %d\n", narrow_cast<int>(trayEvent));
            return "";
        }
    }
}

MinimizePersistence resourceStringToMinimizePersistence(const std::string & str)
{
    if (str == getResourceString(IDS_MINIMIZE_PERSISTENCE_NEVER)) {
        return MinimizePersistence::Never;
    }

    if (str == getResourceString(IDS_MINIMIZE_PERSISTENCE_ALWAYS)) {
        return MinimizePersistence::Always;
    }

    WARNING_PRINTF("Unknown minimize persistence %s\n", str.c_str());
    return MinimizePersistence::None;
}

std::string minimizePersistenceToResourceString(MinimizePersistence minimizePersistence)
{
    switch (minimizePersistence) {
        case MinimizePersistence::Never: return getResourceString(IDS_MINIMIZE_PERSISTENCE_NEVER);
        case MinimizePersistence::Always: return getResourceString(IDS_MINIMIZE_PERSISTENCE_ALWAYS);
        case MinimizePersistence::None:
        default: {
            WARNING_PRINTF("Unknown minimize persistence %d\n", narrow_cast<int>(minimizePersistence));
            return "";
        }
    }
}

} // anonymous namespace
