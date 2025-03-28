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
#include "TrayEvent.h"
#include "Log.h"

std::string trayEventToString(TrayEvent trayEvent)
{
    switch (trayEvent) {
        case TrayEvent::None: return "none";
        case TrayEvent::Open: return "open";
        case TrayEvent::Minimize: return "minimize";
        case TrayEvent::OpenAndMinimize: return "open-and-minimize";

        default: {
            WARNING_PRINTF("error, bad tray event: %d\n", trayEvent);
            return "none";
        }
    }
}

TrayEvent trayEventFromString(const std::string & trayEventString)
{
    if (trayEventString == "none") {
        return TrayEvent::None;
    } else if (trayEventString == "open") {
        return TrayEvent::Open;
    } else if (trayEventString == "minimize") {
        return TrayEvent::Minimize;
    } else if (trayEventString == "open-and-minimize") {
        return TrayEvent::OpenAndMinimize;
    } else {
        WARNING_PRINTF("error, bad tray event string: %s\n", trayEventString.c_str());
        return TrayEvent::None;
    }
}

bool trayEventIncludesOpen(TrayEvent trayEvent)
{
    return (trayEvent == TrayEvent::Open) || (trayEvent == TrayEvent::OpenAndMinimize);
}

bool trayEventIncludesMinimize(TrayEvent trayEvent)
{
    return (trayEvent == TrayEvent::Minimize) || (trayEvent == TrayEvent::OpenAndMinimize);
}
