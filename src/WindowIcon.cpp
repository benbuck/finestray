
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
#include "BrushHandleWrapper.h"
#include "DeviceContextHandleWrapper.h"
#include "Log.h"
#include "StringUtility.h"

namespace WindowIcon
{

HICON get(HWND hwnd)
{
    HICON hicon;

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

    hicon = (HICON)GetClassLongPtr(hwnd, GCLP_HICONSM);
    if (hicon) {
        return hicon;
    }

    hicon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
    if (hicon) {
        return hicon;
    }

    hicon = LoadIcon(nullptr, IDI_APPLICATION);
    if (hicon) {
        return hicon;
    }

    return nullptr;
}

BitmapHandleWrapper bitmap(HWND hwnd)
{
    HICON hicon = get(hwnd);
    if (!hicon) {
        return BitmapHandleWrapper();
    }

    ICONINFOEXA iconInfo;
    memset(&iconInfo, 0, sizeof(iconInfo));
    iconInfo.cbSize = sizeof(ICONINFOEXA);
    if (!GetIconInfoExA(hicon, &iconInfo)) {
        WARNING_PRINTF(
            "failed to get icon info for %#x, GetIconInfoEx() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
        return BitmapHandleWrapper();
    }

    BitmapHandleWrapper iconMaskBitmap(iconInfo.hbmMask);
    BitmapHandleWrapper iconColorBitmap(iconInfo.hbmColor);

    DeviceContextHandleWrapper displayDC(
        CreateICA("DISPLAY", nullptr, nullptr, nullptr),
        DeviceContextHandleWrapper::Created);
    if (!displayDC) {
        WARNING_PRINTF(
            "failed to get desktop information context, CreateICA() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return BitmapHandleWrapper();
    }

    DeviceContextHandleWrapper bitmapDC(CreateCompatibleDC(displayDC), DeviceContextHandleWrapper::Created);
    if (!bitmapDC) {
        WARNING_PRINTF(
            "failed to get desktop device context, CreateCompatibleDC() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return BitmapHandleWrapper();
    }

    int cx = GetSystemMetrics(SM_CXMENUCHECK);
    int cy = GetSystemMetrics(SM_CYMENUCHECK);

    BitmapHandleWrapper bitmap(CreateCompatibleBitmap(displayDC, cx, cy));
    if (!bitmap) {
        WARNING_PRINTF(
            "failed to create bitmap, CreateCompatibleBitmap() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return BitmapHandleWrapper();
    }

    if (!bitmapDC.selectObject(bitmap)) {
        return BitmapHandleWrapper();
    }

    RECT rect = { 0, 0, cx, cy };
    BrushHandleWrapper brush(CreateSolidBrush(GetSysColor(COLOR_MENU)));
    if (!FillRect(bitmapDC, &rect, brush)) {
        WARNING_PRINTF("failed to fill background, FillRect() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    if (!DrawIconEx(bitmapDC, 0, 0, hicon, cx, cy, 0, 0, DI_NORMAL)) {
        WARNING_PRINTF("failed to draw icon, DrawIconEx() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    return bitmap;
}

} // namespace WindowIcon
