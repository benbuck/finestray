
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
#include "WindowIcon.h"
#include "DeviceContextHandleWrapper.h"
#include "Log.h"
#include "StringUtility.h"

namespace WindowIcon
{

namespace
{

bool replaceBitmapMaskColor(HBITMAP hbmp, HBITMAP hmask, COLORREF newColor);

} // anonymous namespace

HICON get(HWND hwnd)
{
    HICON hicon;

    hicon = (HICON)GetClassLongPtr(hwnd, GCLP_HICONSM);
    if (hicon) {
        return hicon;
    }

    hicon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
    if (hicon) {
        return hicon;
    }

    hicon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0);
    if (hicon) {
        return hicon;
    }

    hicon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_BIG, 0);
    if (hicon) {
        return hicon;
    }

    hicon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL2, 0);
    if (hicon) {
        return hicon;
    }

    return nullptr;
}

HBITMAP bitmap(HWND hwnd)
{
    HICON hicon = get(hwnd);
    if (!hicon) {
        return nullptr;
    }

    // return bitmapFromIcon(hicon);

    ICONINFOEXA iconInfo;
    iconInfo.cbSize = sizeof(ICONINFOEXA);
    if (!GetIconInfoExA(hicon, &iconInfo)) {
        return nullptr;
    }

    DWORD menuColor = GetSysColor(COLOR_MENU);
    COLORREF newColor = RGB(GetBValue(menuColor), GetGValue(menuColor), GetRValue(menuColor));
    replaceBitmapMaskColor(iconInfo.hbmColor, iconInfo.hbmMask, newColor);
    DeleteObject(iconInfo.hbmMask);

    HBITMAP scaledBitmap = (HBITMAP)CopyImage(
        iconInfo.hbmColor,
        IMAGE_BITMAP,
        GetSystemMetrics(SM_CXMENUCHECK),
        GetSystemMetrics(SM_CYMENUCHECK),
        LR_COPYDELETEORG);
    if (!scaledBitmap) {
        WARNING_PRINTF("failed to scale bitmap, CopyImage() failed: %s\n", StringUtility::lastErrorString().c_str());
    } else {
        DeleteObject(iconInfo.hbmColor);
        iconInfo.hbmColor = scaledBitmap;
    }

    return iconInfo.hbmColor;
}

namespace
{

bool replaceBitmapMaskColor(HBITMAP hbmp, HBITMAP hmask, COLORREF newColor)
{
    if (!hbmp || !hmask) {
        return false;
    }

    BITMAP maskBitmap;
    if (!GetObjectA(hmask, sizeof(BITMAP), &maskBitmap)) {
        WARNING_PRINTF(
            "failed to get mask bitmap object, GetObject() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    BITMAP colorBitmap;
    if (!GetObjectA(hbmp, sizeof(BITMAP), &colorBitmap)) {
        WARNING_PRINTF(
            "failed to get color bitmap object, GetObject() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    DeviceContextHandleWrapper desktopDC(GetDC(HWND_DESKTOP), DeviceContextHandleWrapper::Referenced);
    if (!desktopDC) {
        WARNING_PRINTF(
            "failed to get desktop device context, GetDC() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    DeviceContextHandleWrapper maskDC(CreateCompatibleDC(desktopDC), DeviceContextHandleWrapper::Created);
    DeviceContextHandleWrapper colorDC(CreateCompatibleDC(desktopDC), DeviceContextHandleWrapper::Created);
    if (!maskDC || !colorDC) {
        WARNING_PRINTF(
            "failed to create compatible device contexts, CreateCompatibleDC() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    if (!maskDC.selectObject(hmask) || !colorDC.selectObject(hbmp)) {
        return false;
    }

    bool replaced = false;
    for (int y = 0; y < maskBitmap.bmHeight; ++y) {
        for (int x = 0; x < maskBitmap.bmWidth; ++x) {
            COLORREF maskPixel = GetPixel(maskDC, x, y);
            if (maskPixel == RGB(255, 255, 255)) {
                SetPixel(colorDC, x, y, newColor);
                replaced = true;
            }
        }
    }

    return replaced;
}

} // anonymous namespace

} // namespace WindowIcon
