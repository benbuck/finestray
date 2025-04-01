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
#include "Settings.h"
#include "AppName.h"
#include "CJsonWrapper.h"
#include "File.h"
#include "Hotkey.h"
#include "Log.h"
#include "Path.h"
#include "StringUtility.h"

// Windows
#include <Windows.h>
#include <shellapi.h>

// Standard library
#include <algorithm>
#include <cstdlib>
#include <regex>

namespace
{

bool autoTrayItemCallback(const cJSON * cjson, void * userData);
bool getBool(const cJSON * cjson, const char * key, bool defaultValue);
double getNumber(const cJSON * cjson, const char * key, double defaultValue);
const char * getString(const cJSON * cjson, const char * key, const char * defaultValue);
void iterateArray(const cJSON * cjson, bool (*callback)(const cJSON *, void *), void * userData);

enum SettingKeys : unsigned int
{
    SK_Version,
    SK_StartWithWindows,
    SK_ShowWindowsInMenu,
    SK_LogToFile,
    SK_MinimizePlacement,
    SK_Executable,
    SK_WindowClass,
    SK_WindowTitle,
    SK_TrayEvent,
    SK_HotkeyMinimize,
    SK_HotkeyMinimizeAll,
    SK_HotkeyRestore,
    SK_HotkeyRestoreAll,
    SK_HotkeyMenu,
    SK_ModifiersOverride,
    SK_PollInterval,
    SK_AutoTray,

    SK_Count
};

constexpr unsigned int versionCurrent_ = 1;
constexpr bool startWithWindowsDefault_ = false;
constexpr bool showWindowsInMenuDefault_ = false;
constexpr bool logToFileDefault_ = false;
constexpr MinimizePlacement minimizePlacementDefault_ = MinimizePlacement::TrayAndMenu;
constexpr char hotkeyMinimizeDefault_[] = "alt ctrl shift down";
constexpr char hotkeyMinimizeAllDefault_[] = "alt ctrl shift right";
constexpr char hotkeyRestoreDefault_[] = "alt ctrl shift up";
constexpr char hotkeyRestoreAllDefault_[] = "alt ctrl shift left";
constexpr char hotkeyMenuDefault_[] = "alt ctrl shift home";
constexpr char modifiersOverrideDefault_[] = "alt ctrl shift";
constexpr unsigned int pollIntervalDefault_ = 500;
const char * settingKeys_[SK_Count] = { "version",
                                        "start-with-windows",
                                        "show-windows-in-menu",
                                        "log-to-file",
                                        "minimize-placement",
                                        "executable",
                                        "window-class",
                                        "window-title",
                                        "tray-event",
                                        "hotkey-minimize",
                                        "hotkey-minimize-all",
                                        "hotkey-restore",
                                        "hotkey-restore-all",
                                        "hotkey-menu",
                                        "modifiers-override",
                                        "poll-interval",
                                        "auto-tray" };

} // anonymous namespace

Settings::Settings()
    : version_(versionCurrent_)
    , startWithWindows_(startWithWindowsDefault_)
    , showWindowsInMenu_(showWindowsInMenuDefault_)
    , logToFile_(logToFileDefault_)
    , minimizePlacement_(minimizePlacementDefault_)
    , hotkeyMinimize_(hotkeyMinimizeDefault_)
    , hotkeyMinimizeAll_(hotkeyMinimizeAllDefault_)
    , hotkeyRestore_(hotkeyRestoreDefault_)
    , hotkeyRestoreAll_(hotkeyRestoreAllDefault_)
    , hotkeyMenu_(hotkeyMenuDefault_)
    , modifiersOverride_(modifiersOverrideDefault_)
    , pollInterval_(pollIntervalDefault_)
{
}

bool Settings::fromJSON(const std::string & json)
{
    CJsonWrapper cjson(cJSON_Parse(json.c_str()));
    if (!cjson) {
        WARNING_PRINTF("failed to parse settings JSON:\n%s\n", cJSON_GetErrorPtr());
        return false;
    }

    DEBUG_PRINTF("parsed settings JSON:\n%s\n", cjson.print().c_str());

    version_ = static_cast<unsigned int>(getNumber(cjson, settingKeys_[SK_Version], static_cast<double>(versionCurrent_)));
    startWithWindows_ = getBool(cjson, settingKeys_[SK_StartWithWindows], startWithWindows_);
    showWindowsInMenu_ = getBool(cjson, settingKeys_[SK_ShowWindowsInMenu], showWindowsInMenu_);
    logToFile_ = getBool(cjson, settingKeys_[SK_LogToFile], logToFile_);

    const std::string & minimizePlacementString =
        getString(cjson, settingKeys_[SK_MinimizePlacement], minimizePlacementToCString(minimizePlacement_));
    minimizePlacement_ = minimizePlacementFromCString(minimizePlacementString.c_str());
    if (minimizePlacement_ == MinimizePlacement::None) {
        WARNING_PRINTF("bad %s argument: %s\n", settingKeys_[SK_MinimizePlacement], minimizePlacementString.c_str());
    }

    hotkeyMinimize_ = getString(cjson, settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_.c_str());
    hotkeyMinimizeAll_ = getString(cjson, settingKeys_[SK_HotkeyMinimizeAll], hotkeyMinimizeAll_.c_str());
    hotkeyRestore_ = getString(cjson, settingKeys_[SK_HotkeyRestore], hotkeyRestore_.c_str());
    hotkeyRestoreAll_ = getString(cjson, settingKeys_[SK_HotkeyRestoreAll], hotkeyRestoreAll_.c_str());
    hotkeyMenu_ = getString(cjson, settingKeys_[SK_HotkeyMenu], hotkeyMenu_.c_str());
    modifiersOverride_ = getString(cjson, settingKeys_[SK_ModifiersOverride], modifiersOverride_.c_str());
    pollInterval_ = static_cast<unsigned int>(
        getNumber(cjson, settingKeys_[SK_PollInterval], static_cast<double>(pollInterval_)));

    const cJSON * autotray = cJSON_GetObjectItemCaseSensitive(cjson, settingKeys_[SK_AutoTray]);
    if (autotray) {
        iterateArray(autotray, autoTrayItemCallback, this);
    }

    normalize();

    return true;
}

std::string Settings::toJSON() const
{
    CJsonWrapper cjson(cJSON_CreateObject());
    if (!cjson) {
        WARNING_PRINTF("failed to create settings JSON object\n");
        return {};
    }

    bool fail = false;

    if (!cJSON_AddNumberToObject(cjson, settingKeys_[SK_Version], static_cast<double>(version_))) {
        fail = true;
    }

    if (!cJSON_AddBoolToObject(cjson, settingKeys_[SK_StartWithWindows], startWithWindows_)) {
        fail = true;
    }

    if (!cJSON_AddBoolToObject(cjson, settingKeys_[SK_ShowWindowsInMenu], showWindowsInMenu_)) {
        fail = true;
    }

    if (!cJSON_AddBoolToObject(cjson, settingKeys_[SK_LogToFile], logToFile_)) {
        fail = true;
    }

    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_MinimizePlacement], minimizePlacementToCString(minimizePlacement_))) {
        fail = true;
    }

    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_HotkeyMinimizeAll], hotkeyMinimizeAll_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_HotkeyRestore], hotkeyRestore_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_HotkeyRestoreAll], hotkeyRestoreAll_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_HotkeyMenu], hotkeyMenu_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_ModifiersOverride], modifiersOverride_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddNumberToObject(cjson, settingKeys_[SK_PollInterval], static_cast<double>(pollInterval_))) {
        fail = true;
    }

    if (!autoTrays_.empty()) {
        cJSON * autotrayArray = cJSON_AddArrayToObject(cjson, settingKeys_[SK_AutoTray]);
        if (!autotrayArray) {
            fail = true;
        } else {
            for (const AutoTray & autoTray : autoTrays_) {
                cJSON * item = cJSON_CreateObject();
                if (!item) {
                    fail = true;
                    break;
                }

                if (!autoTray.executable_.empty() &&
                    !cJSON_AddStringToObject(item, settingKeys_[SK_Executable], autoTray.executable_.c_str())) {
                    fail = true;
                }

                if (!autoTray.windowClass_.empty() &&
                    !cJSON_AddStringToObject(item, settingKeys_[SK_WindowClass], autoTray.windowClass_.c_str())) {
                    fail = true;
                }

                if (!autoTray.windowTitle_.empty() &&
                    !cJSON_AddStringToObject(item, settingKeys_[SK_WindowTitle], autoTray.windowTitle_.c_str())) {
                    fail = true;
                }

                if (!cJSON_AddStringToObject(item, settingKeys_[SK_TrayEvent], trayEventToCString(autoTray.trayEvent_))) {
                    fail = true;
                }

                if (!cJSON_AddItemToArray(autotrayArray, item)) {
                    fail = true;
                }
            }
        }
    }

    if (fail) {
        WARNING_PRINTF("failed to construct json settings\n");
        return {};
    }

    return cjson.print();
}

bool Settings::valid() const
{
    if (version_ != versionCurrent_) {
        return false;
    }

    switch (minimizePlacement_) {
        case MinimizePlacement::Tray:
        case MinimizePlacement::Menu:
        case MinimizePlacement::TrayAndMenu: {
            break;
        }

        case MinimizePlacement::None:
        default: {
            return false;
        }
    }

    if (!Hotkey::valid(hotkeyMinimize_)) {
        return false;
    }

    if (!Hotkey::valid(hotkeyMinimizeAll_)) {
        return false;
    }

    if (!Hotkey::valid(hotkeyRestore_)) {
        return false;
    }

    if (!Hotkey::valid(hotkeyRestoreAll_)) {
        return false;
    }

    if (!Hotkey::valid(hotkeyMenu_)) {
        return false;
    }

    if (!Hotkey::valid(modifiersOverride_)) {
        return false;
    }

    // nothing to validate for poll interval

    if (std::any_of(autoTrays_.begin(), autoTrays_.end(), [](const AutoTray & autoTray) {
            try {
                const std::regex re(autoTray.windowTitle_);
                static_cast<void>(re);
                return false;
            } catch (const std::regex_error &) {
                return true;
            }
        })) {
        return false;
    }

    return true;
}

void Settings::normalize()
{
    version_ = versionCurrent_;

    switch (minimizePlacement_) {
        case MinimizePlacement::Tray:
        case MinimizePlacement::Menu:
        case MinimizePlacement::TrayAndMenu: {
            // no change
            break;
        }

        case MinimizePlacement::None:
        default: {
            DEBUG_PRINTF("Fixing bad minimize placement: %d\n", minimizePlacement_);
            minimizePlacement_ = minimizePlacementDefault_;
            break;
        }
    }

    hotkeyMinimize_ = Hotkey::normalize(hotkeyMinimize_);
    hotkeyMinimizeAll_ = Hotkey::normalize(hotkeyMinimizeAll_);
    hotkeyRestore_ = Hotkey::normalize(hotkeyRestore_);
    hotkeyRestoreAll_ = Hotkey::normalize(hotkeyRestoreAll_);
    hotkeyMenu_ = Hotkey::normalize(hotkeyMenu_);
    modifiersOverride_ = Hotkey::normalize(modifiersOverride_);

    for (std::vector<Settings::AutoTray>::iterator it = autoTrays_.begin(); it != autoTrays_.end();) {
        AutoTray & autoTray = *it;
        if (autoTray.executable_.empty() && autoTray.windowClass_.empty() && autoTray.windowTitle_.empty()) {
            DEBUG_PRINTF("Removing empty auto-tray item\n");
            it = autoTrays_.erase(it);
            continue;
        }

        if (autoTray.trayEvent_ == TrayEvent::None) {
            DEBUG_PRINTF("Changing auto-tray item with no event to minimize\n");
            autoTray.trayEvent_ = TrayEvent::Minimize;
        }

        ++it;
    }
}

void Settings::dump() const
{
#if !defined(NDEBUG)
    DEBUG_PRINTF("Settings:\n");
    DEBUG_PRINTF("\t%s: %u\n", settingKeys_[SK_Version], version_);
    DEBUG_PRINTF("\t%s: %s\n", settingKeys_[SK_StartWithWindows], StringUtility::boolToCString(startWithWindows_));
    DEBUG_PRINTF("\t%s: %s\n", settingKeys_[SK_ShowWindowsInMenu], StringUtility::boolToCString(showWindowsInMenu_));
    DEBUG_PRINTF("\t%s: %s\n", settingKeys_[SK_LogToFile], StringUtility::boolToCString(logToFile_));
    DEBUG_PRINTF("\t%s: %s\n", settingKeys_[SK_MinimizePlacement], minimizePlacementToCString(minimizePlacement_));
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_.c_str());
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_HotkeyMinimizeAll], hotkeyMinimizeAll_.c_str());
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_HotkeyRestore], hotkeyRestore_.c_str());
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_HotkeyRestoreAll], hotkeyRestoreAll_.c_str());
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_HotkeyMenu], hotkeyMenu_.c_str());
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_ModifiersOverride], modifiersOverride_.c_str());
    DEBUG_PRINTF("\t%s: %u\n", settingKeys_[SK_PollInterval], pollInterval_);

    for (const AutoTray & autoTray : autoTrays_) {
        DEBUG_PRINTF("\t%s:\n", settingKeys_[SK_AutoTray]);
        DEBUG_PRINTF("\t\t%s: '%s'\n", settingKeys_[SK_Executable], autoTray.executable_.c_str());
        DEBUG_PRINTF("\t\t%s: '%s'\n", settingKeys_[SK_WindowClass], autoTray.windowClass_.c_str());
        DEBUG_PRINTF("\t\t%s: '%s'\n", settingKeys_[SK_WindowTitle], autoTray.windowTitle_.c_str());
        DEBUG_PRINTF("\t\t%s: '%s'\n", settingKeys_[SK_TrayEvent], trayEventToCString(autoTray.trayEvent_));
    }
#endif
}

void Settings::addAutoTray(AutoTray && autoTray)
{
    autoTrays_.emplace_back(std::move(autoTray));
}

bool Settings::fileExists(const std::string & fileName)
{
    const std::string writeableDir = getWriteableDir();
    if (writeableDir.empty()) {
        return false;
    }

    const std::string fullPath = pathJoin(writeableDir, fileName);
    return ::fileExists(fullPath);
}

namespace
{

bool autoTrayItemCallback(const cJSON * cjson, void * userData)
{
    if (!cjson) {
        WARNING_PRINTF("null cjson\n");
        return false;
    }
    if (!cJSON_IsObject(cjson)) {
        WARNING_PRINTF("bad type for '%s'\n", cjson->string);
        return false;
    }
    Settings * settings = static_cast<Settings *>(userData);
    if (!settings) {
        WARNING_PRINTF("null settings\n");
        return false;
    }

    const char * executable = getString(cjson, settingKeys_[SK_Executable], nullptr);
    const char * windowClass = getString(cjson, settingKeys_[SK_WindowClass], nullptr);
    const char * windowTitle = getString(cjson, settingKeys_[SK_WindowTitle], nullptr);
    const char * trayEvent = getString(cjson, settingKeys_[SK_TrayEvent], nullptr);
    if (executable || windowClass || windowTitle) {
        Settings::AutoTray autoTray;
        autoTray.executable_ = executable ? executable : "";
        autoTray.windowClass_ = windowClass ? windowClass : "";
        autoTray.windowTitle_ = windowTitle ? windowTitle : "";
        autoTray.trayEvent_ = trayEvent ? trayEventFromCString(trayEvent) : TrayEvent::Minimize;

        settings->addAutoTray(std::move(autoTray));
    }

    return true;
}

bool getBool(const cJSON * cjson, const char * key, bool defaultValue)
{
    const cJSON * item = cJSON_GetObjectItemCaseSensitive(cjson, key);
    if (!item) {
        return defaultValue;
    }

    if (!cJSON_IsBool(item)) {
        WARNING_PRINTF("bad type for '%s'\n", item->string);
        return defaultValue;
    }

    return cJSON_IsTrue(item) == 1;
}

double getNumber(const cJSON * cjson, const char * key, double defaultValue)
{
    const cJSON * item = cJSON_GetObjectItemCaseSensitive(cjson, key);
    if (!item) {
        return defaultValue;
    }

    if (!cJSON_IsNumber(item)) {
        WARNING_PRINTF("bad type for '%s'\n", item->string);
        return defaultValue;
    }

    return cJSON_GetNumberValue(item);
}

const char * getString(const cJSON * cjson, const char * key, const char * defaultValue)
{
    const cJSON * item = cJSON_GetObjectItemCaseSensitive(cjson, key);
    if (!item) {
        return defaultValue;
    }

    const char * str = cJSON_GetStringValue(item);
    if (!str) {
        WARNING_PRINTF("bad type for '%s'\n", cjson->string);
        return defaultValue;
    }

    return str;
}

void iterateArray(const cJSON * cjson, bool (*callback)(const cJSON *, void *), void * userData)
{
    if (!cjson) {
        WARNING_PRINTF("null cjson\n");
        return;
    }
    if (!callback) {
        WARNING_PRINTF("null callback\n");
        return;
    }

    if (!cJSON_IsArray(cjson)) {
        WARNING_PRINTF("bad type for '%s'\n", cjson->string);
    } else {
        const int arrSize = cJSON_GetArraySize(cjson);
        for (int i = 0; i < arrSize; ++i) {
            const cJSON * item = cJSON_GetArrayItem(cjson, i);
            if (!callback(item, userData)) {
                break;
            }
        }
    }
}

} // anonymous namespace
