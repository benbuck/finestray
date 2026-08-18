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

// Windows
#include <Windows.h>
#include <combaseapi.h>
#include <winstring.h>

// Undocumented COM interface method vtable slot numbers
enum class VirtualDesktopSlot
{
    MoveViewToDesktop = 4,
    GetDesktopId = 4, // IVirtualDesktop, GetId is slot 4 in all supported layouts
    GetCurrentDesktop = 6,
    GetViewForHwnd = 6,
    CreateDesktop = 10,
    RemoveDesktopLegacy = 11, // Windows 10 1903 through Windows 11 22H2
    RemoveDesktop = 12, // Windows 11 22H2 and later
    SetDesktopName = 15, // Windows 11 22H2 and later
    SetDesktopNameLegacy = 16, // Windows 10 1903 through Windows 11 22H2
};

// ntdll exports RtlGetVersion, but it is not declared in the Windows SDK headers
struct VersionInfo
{
    ULONG size;
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildNumber;
    ULONG platformId;
    WCHAR servicePack[128];
};

extern "C" LONG WINAPI RtlGetVersion(VersionInfo * versionInfo);

constexpr GUID CLSID_ImmersiveShell = { 0xC2F03A33, 0x21F5, 0x47FA, { 0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39 } };

// Virtual desktop service GUID
constexpr GUID CLSID_VirtualDesktopManagerInternal = { 0xC5E0CDCA,
                                                       0x7B6E,
                                                       0x41B2,
                                                       { 0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B } };

// IVirtualDesktopManagerInternal, Windows 11 22H2 and later:
// GetCurrentDesktop = 6, MoveViewToDesktop = 4, CreateDesktop = 10, RemoveDesktop = 12
constexpr GUID IID_VirtualDesktopManagerInternal = { 0x53F5CA0B,
                                                     0x158F,
                                                     0x4124,
                                                     { 0x90, 0x0C, 0x05, 0x71, 0x58, 0x06, 0x0B, 0x27 } };

// IVirtualDesktopManagerInternal, Windows 10 1903 through Windows 11 22H2:
// GetCurrentDesktop = 6, MoveViewToDesktop = 4, CreateDesktop = 10, RemoveDesktop = 11
constexpr GUID IID_VirtualDesktopManagerInternalLegacy = { 0xF31574D6,
                                                           0xB682,
                                                           0x4CDC,
                                                           { 0xBD, 0x56, 0x18, 0x27, 0x86, 0x0A, 0xBE, 0xC6 } };

// IApplicationViewCollection: GetViewForHwnd = 6
constexpr GUID IID_ApplicationViewCollection = { 0x1841C6D7,
                                                 0x4F9D,
                                                 0x42C0,
                                                 { 0xAF, 0x41, 0x87, 0x47, 0x53, 0x8F, 0x10, 0xE5 } };

using FnGetViewForHwnd = HRESULT(STDMETHODCALLTYPE *)(IUnknown * object, HWND hwnd, IUnknown ** view);
using FnMoveViewToDesktop = HRESULT(STDMETHODCALLTYPE *)(IUnknown * object, IUnknown * view, IUnknown * desktop);
using FnGetCurrentDesktop = HRESULT(STDMETHODCALLTYPE *)(IUnknown * object, IUnknown ** desktop);
using FnCreateDesktop = HRESULT(STDMETHODCALLTYPE *)(IUnknown * object, IUnknown ** desktop);
using FnRemoveDesktop = HRESULT(STDMETHODCALLTYPE *)(IUnknown * object, IUnknown * desktop, IUnknown * fallbackDesktop);
using FnSetDesktopName = HRESULT(STDMETHODCALLTYPE *)(IUnknown * object, IUnknown * desktop, HSTRING name);
using FnGetDesktopId = HRESULT(STDMETHODCALLTYPE *)(IUnknown * object, GUID * desktopId);
