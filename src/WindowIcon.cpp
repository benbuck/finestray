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
#include "BitmapHandleWrapper.h"
#include "BrushHandleWrapper.h"
#include "DeviceContextHandleWrapper.h"
#include "HandleWrapper.h"
#include "Helpers.h"
#include "IconHandleWrapper.h"
#include "Log.h"
#include "StringUtility.h"

// Windows
#include <PropIdl.h>
#include <ShObjIdl_core.h>
#include <Windows.h>
#include <appmodel.h>
#include <combaseapi.h>
#include <propkey.h>
#include <propsys.h>
#include <shellapi.h>
#include <wrl/client.h>

// Standard library
#include <cwchar>
#include <span>

using Microsoft::WRL::ComPtr;

namespace
{

bool getWindowAppUserModelId(HWND hwnd, wchar_t * appUserModelId, size_t appUserModelIdSize)
{
    ComPtr<IPropertyStore> propertyStore;
    if (SUCCEEDED(SHGetPropertyStoreForWindow(
            hwnd,
            IID_IPropertyStore,
            reinterpret_cast<void **>(propertyStore.ReleaseAndGetAddressOf())))) {
        PROPVARIANT propVariant;
        PropVariantInit(&propVariant);
        const bool found = SUCCEEDED(propertyStore->GetValue(PKEY_AppUserModel_ID, &propVariant)) &&
            (propVariant.vt == VT_LPWSTR) && propVariant.pwszVal && (wcslen(propVariant.pwszVal) < appUserModelIdSize);
        if (found) {
            wcscpy_s(appUserModelId, appUserModelIdSize, propVariant.pwszVal);
        }
        PropVariantClear(&propVariant);
        if (found) {
            return true;
        }
    }

    DWORD processID = 0;
    if (!GetWindowThreadProcessId(hwnd, &processID)) {
        WARNING_PRINTF("GetWindowThreadProcessId failed for %#x: %s\n", hwnd, StringUtility::lastErrorString().c_str());
    } else {
        const HandleWrapper process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID));
        if (!process) {
            WARNING_PRINTF("OpenProcess() failed: %s\n", StringUtility::lastErrorString().c_str());
        } else {
            UINT32 length = narrow_cast<UINT32>(appUserModelIdSize);
            if (GetApplicationUserModelId(process, &length, appUserModelId) == ERROR_SUCCESS) {
                return true;
            }
        }
    }

    return false;
}

HICON createHIconFromBitmap(HBITMAP hbitmap)
{
    BITMAP bitmap;
    if (!GetObject(hbitmap, sizeof(bitmap), &bitmap) || (bitmap.bmWidth <= 0) || (bitmap.bmHeight <= 0)) {
        return nullptr;
    }

    const LONG cx = bitmap.bmWidth;
    const LONG cy = bitmap.bmHeight;
    const bool hasAlpha = bitmap.bmBitsPixel == 32;

    const DeviceContextHandleWrapper displayDC(GetDC(HWND_DESKTOP), DeviceContextHandleWrapper::Referenced);
    if (!displayDC) {
        return nullptr;
    }

    DeviceContextHandleWrapper colorDC(CreateCompatibleDC(displayDC), DeviceContextHandleWrapper::Created);
    DeviceContextHandleWrapper sourceDC(CreateCompatibleDC(displayDC), DeviceContextHandleWrapper::Created);
    if (!colorDC || !sourceDC) {
        return nullptr;
    }

    BITMAPINFO bitmapInfo;
    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = cx;
    bitmapInfo.bmiHeader.biHeight = -cy;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    BYTE * bits = nullptr;
    const BitmapHandleWrapper colorBitmap(
        CreateDIBSection(displayDC, &bitmapInfo, DIB_RGB_COLORS, reinterpret_cast<void **>(&bits), nullptr, 0));
    if (!colorBitmap || !bits) {
        return nullptr;
    }

    if (!colorDC.selectObject(colorBitmap) || !sourceDC.selectObject(hbitmap)) {
        return nullptr;
    }

    if (!BitBlt(colorDC, 0, 0, cx, cy, sourceDC, 0, 0, SRCCOPY)) {
        WARNING_PRINTF("failed to copy icon bitmap, BitBlt() failed: %s\n", StringUtility::lastErrorString().c_str());
        return nullptr;
    }

    const BitmapHandleWrapper maskBitmap(CreateBitmap(cx, cy, 1, 1, nullptr));
    if (!maskBitmap) {
        return nullptr;
    }

    DeviceContextHandleWrapper maskDC(CreateCompatibleDC(displayDC), DeviceContextHandleWrapper::Created);
    if (!maskDC || !maskDC.selectObject(maskBitmap)) {
        return nullptr;
    }

    const RECT rect = { 0, 0, cx, cy };
    if (hasAlpha) {
        // mask is transparent (white) by default, opaque (black) where alpha is set
        if (!FillRect(maskDC, &rect, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)))) {
            WARNING_PRINTF("failed to fill icon mask, FillRect() failed: %s\n", StringUtility::lastErrorString().c_str());
            return nullptr;
        }

        const std::span<const BYTE> bitsSpan(bits, static_cast<size_t>(cx) * static_cast<size_t>(cy) * 4);
        for (LONG y = 0; y < cy; ++y) {
            const std::span<const BYTE> row =
                bitsSpan.subspan(static_cast<size_t>(y) * static_cast<size_t>(cx) * 4, static_cast<size_t>(cx) * 4);
            for (LONG x = 0; x < cx; ++x) {
                if (row[(static_cast<size_t>(x) * 4) + 3] >= 128) {
                    SetPixelV(maskDC, x, y, 0);
                }
            }
        }
    } else {
        // no alpha channel, icon is fully opaque (black mask)
        if (!FillRect(maskDC, &rect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)))) {
            WARNING_PRINTF("failed to fill icon mask, FillRect() failed: %s\n", StringUtility::lastErrorString().c_str());
            return nullptr;
        }
    }

    ICONINFO iconInfo;
    memset(&iconInfo, 0, sizeof(iconInfo));
    iconInfo.fIcon = TRUE;
    iconInfo.hbmMask = maskBitmap;
    iconInfo.hbmColor = colorBitmap;
    return CreateIconIndirect(&iconInfo);
}

HICON getAppUserModelIdIcon(const wchar_t * appUserModelId)
{
    wchar_t parsingName[1024] = {};
    constexpr wchar_t prefix[] = L"shell:AppsFolder\\";
    constexpr size_t prefixSize = sizeof(prefix) / sizeof(prefix[0]);
    constexpr size_t parsingNameSize = sizeof(parsingName) / sizeof(parsingName[0]);
    if (wcslen(appUserModelId) >= (parsingNameSize - (prefixSize - 1))) {
        return nullptr;
    }
    wcscpy_s(parsingName, parsingNameSize, prefix);
    wcscat_s(parsingName, parsingNameSize, appUserModelId);

    ComPtr<IShellItemImageFactory> imageFactory;
    if (FAILED(SHCreateItemFromParsingName(
            parsingName,
            nullptr,
            IID_IShellItemImageFactory,
            reinterpret_cast<void **>(imageFactory.ReleaseAndGetAddressOf())))) {
        return nullptr;
    }

    const LONG iconSize = narrow_cast<LONG>(GetSystemMetrics(SM_CXSMICON));
    const SIZE size = { iconSize, iconSize };
    HBITMAP hbitmap = nullptr;
    const HRESULT getImageResult = imageFactory->GetImage(
        size,
        static_cast<unsigned int>(SIIGBF_ICONONLY) | static_cast<unsigned int>(SIIGBF_BIGGERSIZEOK),
        &hbitmap);
    if (FAILED(getImageResult) || !hbitmap) {
        return nullptr;
    }

    const BitmapHandleWrapper bitmap(hbitmap);
    return createHIconFromBitmap(hbitmap);
}

} // anonymous namespace

namespace WindowIcon
{

IconHandleWrapper get(HWND hwnd)
{
    // Use the app user model ID to get the same icon the taskbar shows, which is
    // needed for UWP apps and other packaged apps that don't expose a window icon.
    wchar_t appUserModelId[1024];
    if (getWindowAppUserModelId(hwnd, appUserModelId, sizeof(appUserModelId) / sizeof(appUserModelId[0]))) {
        HICON appIcon = getAppUserModelIdIcon(appUserModelId);
        if (appIcon) {
            return { appIcon, IconHandleWrapper::Mode::Created };
        }
    }

    HICON hicon = reinterpret_cast<HICON>(SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0));
    if (hicon) {
        return { hicon, IconHandleWrapper::Mode::Referenced };
    }

    hicon = reinterpret_cast<HICON>(SendMessage(hwnd, WM_GETICON, ICON_BIG, 0));
    if (hicon) {
        return { hicon, IconHandleWrapper::Mode::Referenced };
    }

    hicon = reinterpret_cast<HICON>(SendMessage(hwnd, WM_GETICON, ICON_SMALL2, 0));
    if (hicon) {
        return { hicon, IconHandleWrapper::Mode::Referenced };
    }

    hicon = reinterpret_cast<HICON>(GetClassLongPtr(hwnd, GCLP_HICONSM));
    if (hicon) {
        return { hicon, IconHandleWrapper::Mode::Referenced };
    }

    hicon = reinterpret_cast<HICON>(GetClassLongPtr(hwnd, GCLP_HICON));
    if (hicon) {
        return { hicon, IconHandleWrapper::Mode::Referenced };
    }

    hicon = LoadIcon(nullptr, IDI_APPLICATION);
    if (hicon) {
        return { hicon, IconHandleWrapper::Mode::Referenced };
    }

    return {};
}

BitmapHandleWrapper bitmap(HWND hwnd)
{
    const IconHandleWrapper icon = get(hwnd);
    if (!icon) {
        return {};
    }

    HICON hicon = icon;

    ICONINFOEXA iconInfo;
    memset(&iconInfo, 0, sizeof(iconInfo));
    iconInfo.cbSize = sizeof(ICONINFOEXA);
    if (!GetIconInfoExA(hicon, &iconInfo)) {
        WARNING_PRINTF(
            "failed to get icon info for %#x, GetIconInfoEx() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
        return {};
    }

    const BitmapHandleWrapper iconMaskBitmap(iconInfo.hbmMask);
    const BitmapHandleWrapper iconColorBitmap(iconInfo.hbmColor);

    const DeviceContextHandleWrapper displayDC(
        CreateICA("DISPLAY", nullptr, nullptr, nullptr),
        DeviceContextHandleWrapper::Created);
    if (!displayDC) {
        WARNING_PRINTF(
            "failed to get desktop information context, CreateICA() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return {};
    }

    DeviceContextHandleWrapper bitmapDC(CreateCompatibleDC(displayDC), DeviceContextHandleWrapper::Created);
    if (!bitmapDC) {
        WARNING_PRINTF(
            "failed to get desktop device context, CreateCompatibleDC() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return {};
    }

    const int cx = GetSystemMetrics(SM_CXMENUCHECK);
    const int cy = GetSystemMetrics(SM_CYMENUCHECK);

    BitmapHandleWrapper bitmap(CreateCompatibleBitmap(displayDC, cx, cy));
    if (!bitmap) {
        WARNING_PRINTF(
            "failed to create bitmap, CreateCompatibleBitmap() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return {};
    }

    if (!bitmapDC.selectObject(bitmap)) {
        return {};
    }

    RECT const rect = { 0, 0, cx, cy };
    const BrushHandleWrapper brush(CreateSolidBrush(GetSysColor(COLOR_MENU)));
    if (!FillRect(bitmapDC, &rect, brush)) {
        WARNING_PRINTF("failed to fill background, FillRect() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    if (!DrawIconEx(bitmapDC, 0, 0, hicon, cx, cy, 0, nullptr, DI_NORMAL)) {
        WARNING_PRINTF("failed to draw icon, DrawIconEx() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    return bitmap;
}

} // namespace WindowIcon
