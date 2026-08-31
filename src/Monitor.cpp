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
#include "Monitor.h"
#include "Helpers.h"
#include "Log.h"
#include "StringUtility.h"

bool monitorGetInfo(HWND hwnd, MonitorInfo & monitorInfo)
{
    HMONITOR hmonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!hmonitor) {
        WARNING_PRINTF("failed to get monitor from window: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    MONITORINFOEXA monitorInfoEx = {};
    monitorInfoEx.cbSize = sizeof(monitorInfoEx);
    if (!GetMonitorInfoA(hmonitor, &monitorInfoEx)) {
        WARNING_PRINTF("failed to get monitor info: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    DEVMODEA deviceMode = {};
    deviceMode.dmSize = sizeof(deviceMode);
    if (!EnumDisplaySettingsA(monitorInfoEx.szDevice, ENUM_CURRENT_SETTINGS, &deviceMode)) {
        WARNING_PRINTF("failed to get display settings: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    monitorInfo.logicalOrigin.x = monitorInfoEx.rcMonitor.left;
    monitorInfo.logicalOrigin.y = monitorInfoEx.rcMonitor.top;
    monitorInfo.logicalWidth = monitorInfoEx.rcMonitor.right - monitorInfoEx.rcMonitor.left;
    monitorInfo.logicalHeight = monitorInfoEx.rcMonitor.bottom - monitorInfoEx.rcMonitor.top;

    monitorInfo.physicalOrigin.x = deviceMode.dmPosition.x;
    monitorInfo.physicalOrigin.y = deviceMode.dmPosition.y;
    monitorInfo.physicalWidth = narrow_cast<LONG>(deviceMode.dmPelsWidth);
    monitorInfo.physicalHeight = narrow_cast<LONG>(deviceMode.dmPelsHeight);

    if ((monitorInfo.logicalWidth <= 0) || (monitorInfo.logicalHeight <= 0) || (monitorInfo.physicalWidth <= 0) ||
        (monitorInfo.physicalHeight <= 0)) {
        WARNING_PRINTF("invalid monitor geometry\n");
        return false;
    }

    return true;
}

POINT monitorPointPhysicalToLogical(POINT physical, const MonitorInfo & monitorInfo) noexcept
{
    return { monitorInfo.logicalOrigin.x +
                 narrow_cast<LONG>(MulDiv(
                     physical.x - monitorInfo.physicalOrigin.x,
                     narrow_cast<int>(monitorInfo.logicalWidth),
                     narrow_cast<int>(monitorInfo.physicalWidth))),
             monitorInfo.logicalOrigin.y +
                 narrow_cast<LONG>(MulDiv(
                     physical.y - monitorInfo.physicalOrigin.y,
                     narrow_cast<int>(monitorInfo.logicalHeight),
                     narrow_cast<int>(monitorInfo.physicalHeight))) };
}
