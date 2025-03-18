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

    // BITMAP colorBitmap;
    // if (!GetObjectA(hbmp, sizeof(BITMAP), &colorBitmap)) {
    //     WARNING_PRINTF(
    //         "failed to get color bitmap object, GetObject() failed: %s\n",
    //         StringUtility::lastErrorString().c_str());
    //     return false;
    // }

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

HBITMAP scaleBitmap(HBITMAP hbmp, int width, int height)
{
    HBITMAP scaledBitmap = (HBITMAP)CopyImage(hbmp, IMAGE_BITMAP, width, height, LR_COPYDELETEORG);
    if (!scaledBitmap) {
        WARNING_PRINTF("failed to scale bitmap, CopyImage() failed: %s\n", StringUtility::lastErrorString().c_str());
        return nullptr;
    }
    return scaledBitmap;
}
