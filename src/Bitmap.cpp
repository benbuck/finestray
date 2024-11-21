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

#include "Bitmap.h"

// App
#include "DeviceContextHandleWrapper.h"
#include "Log.h"
#include "StringUtility.h"

// Standard library
#include <vector>

HBITMAP getResourceBitmap(unsigned int id)
{
    HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(nullptr);
    HBITMAP bitmap = (HBITMAP)LoadImageA(hinstance, MAKEINTRESOURCEA(id), IMAGE_BITMAP, 0, 0, 0);
    if (!bitmap) {
        WARNING_PRINTF(
            "failed to load resources bitmap %u, LoadImage() failed: %s\n",
            id,
            StringUtility::lastErrorString().c_str());
    }

    return bitmap;
}

bool replaceBitmapColor(HBITMAP hbmp, COLORREF oldColor, COLORREF newColor)
{
    if (!hbmp) {
        return false;
    }

    DeviceContextHandleWrapper hdc(GetDC(HWND_DESKTOP), DeviceContextHandleWrapper::Referenced);

    BITMAP bitmap;
    memset(&bitmap, 0, sizeof(bitmap));
    if (!GetObject(hbmp, sizeof(bitmap), &bitmap)) {
        WARNING_PRINTF("failed to get bitmap object, GetObject() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    BITMAPINFO bitmapInfo;
    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = bitmap.bmWidth;
    bitmapInfo.bmiHeader.biHeight = bitmap.bmHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;

    std::vector<COLORREF> pixels(bitmap.bmWidth * bitmap.bmHeight);
    if (!GetDIBits(hdc, hbmp, 0, bitmap.bmHeight, &pixels[0], &bitmapInfo, DIB_RGB_COLORS)) {
        WARNING_PRINTF("failed to get bitmap bits, GetDIBits() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    bool replaced = false;
    for (COLORREF & pixelColor : pixels) {
        if (pixelColor == oldColor) {
            pixelColor = newColor;
        }
    }

    if (!SetDIBits(hdc, hbmp, 0, bitmap.bmHeight, &pixels[0], &bitmapInfo, DIB_RGB_COLORS)) {
        WARNING_PRINTF("failed to set bitmap bits, SetDIBits() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    return replaced;
}
