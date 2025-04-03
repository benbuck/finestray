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
#include "Bitmap.h"
#include "DeviceContextHandleWrapper.h"
#include "Helpers.h"
#include "Log.h"
#include "StringUtility.h"

// Standard library
#include <vector>

namespace Bitmap
{

BitmapHandleWrapper getResource(unsigned int id)
{
    HINSTANCE hinstance = GetModuleHandle(nullptr);
    BitmapHandleWrapper bitmap(static_cast<HBITMAP>(LoadImageA(hinstance, MAKEINTRESOURCEA(id), IMAGE_BITMAP, 0, 0, 0)));
    if (!bitmap) {
        WARNING_PRINTF(
            "failed to load resources bitmap %u, LoadImage() failed: %s\n",
            id,
            StringUtility::lastErrorString().c_str());
    }

    return bitmap;
}

bool replaceColor(const BitmapHandleWrapper & bitmap, COLORREF oldColor, COLORREF newColor)
{
    if (!bitmap) {
        return false;
    }

    const DeviceContextHandleWrapper desktopDC(GetDC(HWND_DESKTOP), DeviceContextHandleWrapper::Referenced);

    BITMAP bm;
    memset(&bm, 0, sizeof(bm));
    if (!GetObject(bitmap, sizeof(bm), &bm)) {
        WARNING_PRINTF("failed to get bm object, GetObject() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    BITMAPINFO bitmapInfo;
    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = bm.bmWidth;
    bitmapInfo.bmiHeader.biHeight = bm.bmHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;

    std::vector<COLORREF> pixels(static_cast<size_t>(bm.bmWidth) * static_cast<size_t>(bm.bmHeight));
    if (!GetDIBits(desktopDC, bitmap, 0, narrow_cast<UINT>(bm.bmHeight), pixels.data(), &bitmapInfo, DIB_RGB_COLORS)) {
        WARNING_PRINTF("failed to get bitmap bits, GetDIBits() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    bool replaced = false;
    for (COLORREF & pixelColor : pixels) {
        if (pixelColor == oldColor) {
            pixelColor = newColor;
            replaced = true;
        }
    }

    if (!SetDIBits(desktopDC, bitmap, 0, narrow_cast<UINT>(bm.bmHeight), pixels.data(), &bitmapInfo, DIB_RGB_COLORS)) {
        WARNING_PRINTF("failed to set bitmap bits, SetDIBits() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    return replaced;
}

} // namespace Bitmap
