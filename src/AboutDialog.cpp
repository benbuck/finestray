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

#include "AboutDialog.h"

// App
#include "Helpers.h"
#include "Log.h"
#include "Resource.h"
#include "StringUtility.h"

// Standard library
#include <string>

namespace
{
bool aboutDialogOpen_;
} // anonymous namespace

void showAboutDialog(HWND hwnd)
{
    INFO_PRINTF("showing about dialog\n");

    if (aboutDialogOpen_) {
        WARNING_PRINTF("about dialog already open\n");
        return;
    }

    const std::string & aboutTextStr = getResourceString(IDS_ABOUT_TEXT);
    const std::string & aboutCaptionStr = getResourceString(IDS_ABOUT_CAPTION);
    UINT type = MB_OK | MB_ICONINFORMATION | MB_TASKMODAL;

    aboutDialogOpen_ = true;
    int result = MessageBoxA(hwnd, aboutTextStr.c_str(), aboutCaptionStr.c_str(), type);
    aboutDialogOpen_ = false;

    if (!result) {
        WARNING_PRINTF(
            "could not create about dialog, MessageBoxA() failed: %s\n",
            StringUtility::lastErrorString().c_str());
    }
}
