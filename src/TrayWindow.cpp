// Copyright 2020 Benbuck Nason

#include "TrayWindow.h"

#include "TrayIcon.h"

#include <list>
#include <memory>

struct IconData
{
    IconData(HWND hwnd, TrayIcon * trayIcon) : hwnd_(hwnd), trayIcon_(trayIcon) {}
    HWND hwnd_;
    std::unique_ptr<TrayIcon> trayIcon_;
};

typedef std::list<IconData> TrayIcons;
static TrayIcons trayIcons_;

static TrayIcons::iterator find(HWND hwnd);
static bool add(HWND hwnd, HWND messageWnd);
static bool remove(HWND hwnd);
static HICON getIcon(HWND hwnd);

void trayWindowMinimize(HWND hwnd, HWND messageWnd)
{
    // hide window
    ShowWindow(hwnd, SW_HIDE);

    // add tray icon
    if (find(hwnd) == trayIcons_.end()) {
        if (!add(hwnd, messageWnd)) {
            // restore window on failure
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            return;
        }
    }
}

void trayWindowRestore(HWND hwnd)
{
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    remove(hwnd);
}

void trayWindowClose(HWND hwnd)
{
    // close the window
    PostMessage(hwnd, WM_CLOSE, 0, 0);

    // wait for close to complete
    unsigned int counter = 0;
    do {
        Sleep(100);
        ++counter;
        if (counter >= 10) {
            break;
        }
    } while (IsWindow(hwnd));

    // closed successfully
    if (!IsWindow(hwnd)) {
        remove(hwnd);
    }
}

void trayWindowRefresh(HWND hwnd)
{
    // find the window
    TrayIcons::iterator it = find(hwnd);
    if (it == trayIcons_.end()) {
        return;
    }

    // refresh the window
    if (!IsWindow(hwnd) || IsWindowVisible(hwnd)) {
        remove(hwnd);
    } else {
        it->trayIcon_->refresh();
    }
}

void trayWindowAddAll()
{
    for (TrayIcons::const_iterator cit = trayIcons_.cbegin(); cit != trayIcons_.cend(); ++cit) {
        IconData const & iconData = *cit;
        add(iconData.hwnd_, iconData.trayIcon_->hwnd());
    }
}

void trayWindowRestoreAll()
{
    for (TrayIcons::const_iterator cit = trayIcons_.cbegin(); cit != trayIcons_.cend(); ++cit) {
        IconData const & iconData = *cit;
        ShowWindow(iconData.hwnd_, SW_SHOW);
        SetForegroundWindow(iconData.hwnd_);
    }

    trayIcons_.clear();
}

HWND trayWindowGetFromID(UINT id)
{
    for (TrayIcons::const_iterator cit = trayIcons_.cbegin(); cit != trayIcons_.cend(); ++cit) {
        IconData const & iconData = *cit;
        if (iconData.trayIcon_->id() == id) {
            return iconData.hwnd_;
        }
    }

    return NULL;
}

HWND trayWindowGetLast()
{
    if (trayIcons_.empty()) {
        return NULL;
    }

    return trayIcons_.back().hwnd_;
}

TrayIcons::iterator find(HWND hwnd)
{
    for (TrayIcons::iterator it = trayIcons_.begin(); it != trayIcons_.end(); ++it) {
        IconData const & iconData = *it;
        if (iconData.hwnd_ == hwnd) {
            return it;
        }
    }

    return trayIcons_.end();
}

bool add(HWND hwnd, HWND messageWnd)
{
    // make sure window isn't already tracked
    TrayIcons::iterator it = find(hwnd);
    if (it != trayIcons_.end()) {
        return false;
    }

    // add the window
    trayIcons_.emplace_back(IconData(hwnd, new TrayIcon()));
    return trayIcons_.back().trayIcon_->create(messageWnd, WM_TRAYWINDOW, getIcon(hwnd));
}

bool remove(HWND hwnd)
{
    // find the window
    TrayIcons::iterator it = find(hwnd);
    if (it == trayIcons_.end()) {
        return false;
    }

    // remove the window
    trayIcons_.erase(it);
    return true;
}

HICON getIcon(HWND hwnd)
{
    HICON icon;

    icon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0);
    if (icon) {
        return icon;
    }

    icon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL2, 0);
    if (icon) {
        return icon;
    }

    icon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_BIG, 0);
    if (icon) {
        return icon;
    }

    icon = (HICON)GetClassLongPtr(hwnd, GCLP_HICONSM);
    if (icon) {
        return icon;
    }

    icon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
    if (icon) {
        return icon;
    }

    return LoadIcon(NULL, IDI_WINLOGO);
}
