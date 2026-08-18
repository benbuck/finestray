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
#include "VirtualDesktop.h"
#include "Helpers.h"
#include "Log.h"
#include "WindowsUndocumented.h"

// Windows
#include <ShObjIdl.h>
#include <servprov.h>
#include <winstring.h>
#include <wrl/client.h>

// Standard library
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <span>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace
{

ComPtr<IUnknown> internalManager_;
ComPtr<IUnknown> viewCollection_;
ComPtr<IVirtualDesktopManager> virtualDesktopManager_;
ComPtr<IUnknown> hiddenDesktop_;
GUID hiddenDesktopGuid_ {};
std::vector<HWND> hiddenWindows_;
VirtualDesktopSlot removeDesktopSlot_ = VirtualDesktopSlot::GetViewForHwnd;
VirtualDesktopSlot setDesktopNameSlot_ = VirtualDesktopSlot::GetViewForHwnd;
bool initialized_ = false;
bool desktopRemovalFailed_ = false;

constexpr GUID kNullGuid = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

bool isNullGuid(const GUID & guid) noexcept
{
    return IsEqualGUID(guid, kNullGuid);
}

constexpr PCWSTR kHiddenDesktopName = L"Finestray";

// largest vtable slot accessed, plus one
constexpr std::size_t kVTableSlotCount = static_cast<std::size_t>(VirtualDesktopSlot::SetDesktopNameLegacy) + 1;

template <class T>
T getMethod(const IUnknown * object, VirtualDesktopSlot slot) noexcept
{
    if (!object) {
        return nullptr;
    }

    T * vtbl = *reinterpret_cast<T * const *>(object);
    if (!vtbl) {
        return nullptr;
    }

    const std::span<T> methods(vtbl, kVTableSlotCount);
    return methods[static_cast<std::size_t>(slot)];
}

unsigned int getWindowsBuild() noexcept
{
    VersionInfo versionInfo = {};
    versionInfo.size = sizeof(versionInfo);
    if (RtlGetVersion(&versionInfo) != 0) {
        return 0;
    }
    return versionInfo.buildNumber;
}

void removeHiddenDesktop() noexcept
{
    if (!hiddenDesktop_ || desktopRemovalFailed_) {
        return;
    }

    ComPtr<IUnknown> currentDesktop;
    const HRESULT hr = getMethod<FnGetCurrentDesktop>(internalManager_.Get(), VirtualDesktopSlot::GetCurrentDesktop)(
        internalManager_.Get(),
        currentDesktop.ReleaseAndGetAddressOf());
    if (FAILED(hr) || !currentDesktop) {
        return;
    }

    const HRESULT removeResult = getMethod<FnRemoveDesktop>(
        internalManager_.Get(),
        removeDesktopSlot_)(internalManager_.Get(), hiddenDesktop_.Get(), currentDesktop.Get());
    if (FAILED(removeResult)) {
        WARNING_PRINTF("failed to remove hidden virtual desktop, keeping it for reuse: 0x%08X\n", removeResult);
        desktopRemovalFailed_ = true;
        return;
    }

    hiddenDesktop_ = nullptr;
    hiddenDesktopGuid_ = kNullGuid;
}

void nameHiddenDesktop() noexcept
{
    if (!hiddenDesktop_) {
        return;
    }

    HSTRING_HEADER header = {};
    HSTRING name = nullptr;
    const HRESULT createResult =
        WindowsCreateStringReference(kHiddenDesktopName, narrow_cast<UINT32>(wcslen(kHiddenDesktopName)), &header, &name);
    if (FAILED(createResult)) {
        WARNING_PRINTF("failed to create hidden virtual desktop name string: 0x%08X\n", createResult);
        return;
    }

    const HRESULT nameResult = getMethod<FnSetDesktopName>(
        internalManager_.Get(),
        setDesktopNameSlot_)(internalManager_.Get(), hiddenDesktop_.Get(), name);
    if (FAILED(nameResult)) {
        WARNING_PRINTF("failed to name hidden virtual desktop: 0x%08X\n", nameResult);
    }

    WindowsDeleteString(name);
}

} // anonymous namespace

namespace VirtualDesktop
{

bool start()
{
    if (initialized_) {
        return true;
    }
    initialized_ = true;

    ComPtr<IServiceProvider> shell;
    HRESULT hr = CoCreateInstance(
        CLSID_ImmersiveShell,
        nullptr,
        CLSCTX_LOCAL_SERVER,
        IID_IServiceProvider,
        reinterpret_cast<void **>(shell.ReleaseAndGetAddressOf()));
    if (FAILED(hr)) {
        WARNING_PRINTF("failed to create ImmersiveShell, CoCreateInstance() failed: 0x%08X\n", hr);
        return false;
    }

    // Windows 11 22H2 (build 22621) and later expose the modern interface layout
    if (getWindowsBuild() >= 22621) {
        hr = shell->QueryService(
            CLSID_VirtualDesktopManagerInternal,
            IID_VirtualDesktopManagerInternal,
            reinterpret_cast<void **>(internalManager_.ReleaseAndGetAddressOf()));
        if (SUCCEEDED(hr)) {
            removeDesktopSlot_ = VirtualDesktopSlot::RemoveDesktop;
            setDesktopNameSlot_ = VirtualDesktopSlot::SetDesktopName;
        }
    }

    if (!internalManager_) {
        hr = shell->QueryService(
            CLSID_VirtualDesktopManagerInternal,
            IID_VirtualDesktopManagerInternalLegacy,
            reinterpret_cast<void **>(internalManager_.ReleaseAndGetAddressOf()));
        if (SUCCEEDED(hr)) {
            removeDesktopSlot_ = VirtualDesktopSlot::RemoveDesktopLegacy;
            setDesktopNameSlot_ = VirtualDesktopSlot::SetDesktopNameLegacy;
        }
    }

    if (!internalManager_) {
        WARNING_PRINTF("failed to query virtual desktop manager internal service\n");
        return false;
    }

    hr = shell->QueryService(
        IID_ApplicationViewCollection,
        IID_ApplicationViewCollection,
        reinterpret_cast<void **>(viewCollection_.ReleaseAndGetAddressOf()));
    if (FAILED(hr) || !viewCollection_) {
        WARNING_PRINTF("failed to query application view collection service: 0x%08X\n", hr);
        return false;
    }

    // IVirtualDesktopManager (documented) is used to detect when the hidden
    // virtual desktop is removed by the user. It is registered as an in-proc
    // COM server (twinapi.dll), not a local server.
    hr = CoCreateInstance(
        CLSID_VirtualDesktopManager,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IVirtualDesktopManager,
        reinterpret_cast<void **>(virtualDesktopManager_.ReleaseAndGetAddressOf()));
    if (FAILED(hr) || !virtualDesktopManager_) {
        WARNING_PRINTF("failed to create virtual desktop manager: 0x%08X\n", hr);
    }

    initialized_ = true;

    return true;
}

void stop() noexcept
{
    removeHiddenDesktop();

    hiddenWindows_.clear();
    hiddenDesktopGuid_ = kNullGuid;
    desktopRemovalFailed_ = false;
    viewCollection_ = nullptr;
    internalManager_ = nullptr;
    virtualDesktopManager_ = nullptr;
    initialized_ = false;
}

bool minimize(HWND hwnd)
{
    if (!start()) {
        return false;
    }

    IUnknown * view = nullptr;
    HRESULT hr = getMethod<FnGetViewForHwnd>(
        viewCollection_.Get(),
        VirtualDesktopSlot::GetViewForHwnd)(viewCollection_.Get(), hwnd, &view);
    if (FAILED(hr) || !view) {
        WARNING_PRINTF("failed to get application view for %#x: 0x%08X\n", hwnd, hr);
        return false;
    }

    // If the hidden desktop was removed by the user, reset state so a fresh desktop is created
    if (hiddenDesktop_ && !hiddenWindows_.empty()) {
        checkHiddenDesktopRemoved();
    }

    const bool createdDesktop = !hiddenDesktop_;
    if (createdDesktop) {
        hr = getMethod<FnCreateDesktop>(internalManager_.Get(), VirtualDesktopSlot::CreateDesktop)(
            internalManager_.Get(),
            &hiddenDesktop_);
        if (FAILED(hr) || !hiddenDesktop_) {
            WARNING_PRINTF("failed to create hidden virtual desktop: 0x%08X\n", hr);
            view->Release();
            return false;
        }
        nameHiddenDesktop();

        // Capture the hidden desktop's GUID for later removal detection. A
        // window does not reliably report the hidden desktop's GUID right after
        // being moved, so read the GUID from the desktop object itself.
        GUID desktopGuid = {};
        const HRESULT guidResult = getMethod<FnGetDesktopId>(hiddenDesktop_.Get(), VirtualDesktopSlot::GetDesktopId)(
            hiddenDesktop_.Get(),
            &desktopGuid);
        if (SUCCEEDED(guidResult)) {
            hiddenDesktopGuid_ = desktopGuid;
        } else {
            WARNING_PRINTF("failed to get hidden virtual desktop GUID: 0x%08X\n", guidResult);
        }
    }

    hr = getMethod<FnMoveViewToDesktop>(
        internalManager_.Get(),
        VirtualDesktopSlot::MoveViewToDesktop)(internalManager_.Get(), view, hiddenDesktop_.Get());
    const bool moved = SUCCEEDED(hr);
    if (!moved) {
        WARNING_PRINTF("failed to move view to hidden virtual desktop: 0x%08X\n", hr);
        if (createdDesktop) {
            removeHiddenDesktop();
        }
    }
    view->Release();

    if (moved) {
        hiddenWindows_.push_back(hwnd);
    }

    return moved;
}

bool restore(HWND hwnd)
{
    if (!initialized_) {
        return false;
    }

    ComPtr<IUnknown> view;
    HRESULT hr = getMethod<FnGetViewForHwnd>(
        viewCollection_.Get(),
        VirtualDesktopSlot::GetViewForHwnd)(viewCollection_.Get(), hwnd, view.ReleaseAndGetAddressOf());
    if (FAILED(hr) || !view) {
        WARNING_PRINTF("failed to get application view for %#x: 0x%08X\n", hwnd, hr);
        return false;
    }

    ComPtr<IUnknown> currentDesktop;
    hr = getMethod<FnGetCurrentDesktop>(internalManager_.Get(), VirtualDesktopSlot::GetCurrentDesktop)(
        internalManager_.Get(),
        currentDesktop.ReleaseAndGetAddressOf());
    if (FAILED(hr) || !currentDesktop) {
        WARNING_PRINTF("failed to get current virtual desktop: 0x%08X\n", hr);
        view->Release();
        return false;
    }

    hr = getMethod<FnMoveViewToDesktop>(
        internalManager_.Get(),
        VirtualDesktopSlot::MoveViewToDesktop)(internalManager_.Get(), view.Get(), currentDesktop.Get());
    const bool moved = SUCCEEDED(hr);
    if (!moved) {
        WARNING_PRINTF("failed to move view to current virtual desktop: 0x%08X\n", hr);
    }

    if (moved) {
        if (const std::vector<HWND>::iterator it = std::ranges::find(hiddenWindows_, hwnd); it != hiddenWindows_.end()) {
            hiddenWindows_.erase(it);
        }
        if (hiddenWindows_.empty()) {
            removeHiddenDesktop();
            hiddenDesktopGuid_ = kNullGuid;
        }
    }

    return moved;
}

bool isWindowOnCurrentDesktop(HWND hwnd)
{
    if (!initialized_ && !start()) {
        WARNING_PRINTF("failed to start virtual desktop services, assuming window %#x is on the current desktop\n", hwnd);
        return true;
    }

    if (!virtualDesktopManager_) {
        DEBUG_PRINTF("virtual desktop manager unavailable, assuming window %#x is on the current desktop\n", hwnd);
        return true;
    }

    BOOL onCurrentDesktop = FALSE;
    const HRESULT hr = virtualDesktopManager_->IsWindowOnCurrentVirtualDesktop(hwnd, &onCurrentDesktop);
    if (FAILED(hr)) {
        WARNING_PRINTF("failed to determine desktop for window %#x: 0x%08X\n", hwnd, hr);
        return true;
    }

    return onCurrentDesktop != FALSE;
}

std::vector<HWND> checkHiddenDesktopRemoved()
{
    std::vector<HWND> affected;
    if (!hiddenDesktop_ || hiddenWindows_.empty() || !virtualDesktopManager_ || isNullGuid(hiddenDesktopGuid_)) {
        return affected;
    }

    // If the hidden desktop was removed, Windows moves its windows to another
    // desktop, so the windows no longer report the hidden desktop's GUID
    for (HWND hwnd : hiddenWindows_) {
        GUID desktopGuid = {};
        const HRESULT hr = virtualDesktopManager_->GetWindowDesktopId(hwnd, &desktopGuid);
        if (FAILED(hr)) {
            // window no longer available, skip it
            continue;
        }
        if (IsEqualGUID(desktopGuid, hiddenDesktopGuid_)) {
            // window is still on the hidden desktop, so the desktop still exists
            return affected;
        }
    }

    WARNING_PRINTF("hidden virtual desktop was removed by the user\n");
    affected = hiddenWindows_;
    hiddenWindows_.clear();
    hiddenDesktop_ = nullptr;
    hiddenDesktopGuid_ = kNullGuid;
    desktopRemovalFailed_ = false;

    return affected;
}

} // namespace VirtualDesktop
