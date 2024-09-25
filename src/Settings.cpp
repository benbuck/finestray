// Copyright 2020 Benbuck Nason

#include "Settings.h"

// App
#include "DebugPrint.h"
#include "File.h"

// argh
#include <argh.h>

// cJSON
#include <cJSON.h>

// Windows
#include <Windows.h>
#include <shellapi.h>
#include <stdlib.h>

// static bool getBool(const cJSON * cjson, const char * key, bool defaultValue);
static double getNumber(const cJSON * cjson, const char * key, double defaultValue);
static const char * getString(const cJSON * cjson, const char * key, const char * defaultValue);
static void iterateArray(const cJSON * cjson, bool (*callback)(const cJSON *, void *), void *);

enum SettingKeys : unsigned int
{
    SK_AutoTray,
    SK_Executable,
    SK_WindowClass,
    SK_WindowTitle,
    SK_HotkeyMinimize,
    SK_HotkeyRestore,
    SK_ModifiersOverride,
    SK_PollInterval,

    SK_Count
};

static const char hotkeyMinimizeDefault_[] = "alt+ctrl+shift+down";
static const char hotkeyRestoreDefault_[] = "alt+ctrl+shift+up";
static const char modifiersOverrideDefault_[] = "alt+ctrl+shift";
static const unsigned int pollIntervalDefault_ = 500;
static const char * settingKeys_[SK_Count] = { "auto-tray",          "executable",      "window-class",
                                               "window-title",       "hotkey-minimize", "hotkey-restore",
                                               "modifiers-override", "poll-interval" };

Settings::Settings()
    : autoTrays_()
    , hotkeyMinimize_(hotkeyMinimizeDefault_)
    , hotkeyRestore_(hotkeyRestoreDefault_)
    , modifiersOverride_(modifiersOverrideDefault_)
    , pollInterval_(pollIntervalDefault_)
{
}

Settings::~Settings()
{
}

bool Settings::readFromFile(const std::string & fileName)
{
    DEBUG_PRINTF("Reading settings from file: %s\n", fileName.c_str());

    std::string json = fileRead(fileName);
    if (json.empty()) {
        return false;
    }

    return parseJson(json);
}

bool Settings::parseCommandLine(int argc, const char * const * argv)
{
    argh::parser args;
    for (const char * settingKey : settingKeys_) {
        args.add_param(settingKey);
    }
    args.parse(argc, argv);

    if (!args.flags().empty()) {
        DEBUG_PRINTF("error, unexpected command line flags:\n");
        for (const std::string & flag : args.flags()) {
            DEBUG_PRINTF("\t%s\n", flag.c_str());
            (void)flag;
        }
        return false;
    }

    if (args.pos_args().size() > 1) {
        DEBUG_PRINTF("error, unexpected command line arguments:\n");
        for (unsigned int pa = 1; pa < args.pos_args().size(); ++pa) {
            DEBUG_PRINTF("\t%s\n", args.pos_args()[pa].c_str());
        }
        return false;
    }

    // note: "auto-tray" options are not supported on the command line, only in json, since the
    // complexity of the syntax to support the various selector strings would be unwieldy

    hotkeyMinimize_ = args(settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_).str();
    hotkeyRestore_ = args(settingKeys_[SK_HotkeyRestore], hotkeyRestore_).str();
    modifiersOverride_ = args(settingKeys_[SK_ModifiersOverride], modifiersOverride_).str();

    argh::string_stream pollIntervalArg = args(settingKeys_[SK_PollInterval], pollInterval_);
    if (!(pollIntervalArg >> pollInterval_)) {
        DEBUG_PRINTF("error, bad %s argument: %s\n", settingKeys_[SK_PollInterval], pollIntervalArg.str().c_str());
    }

    sanitize();

    return true;
}

bool Settings::writeToFile(const std::string & fileName)
{
    DEBUG_PRINTF("Writing settings to file %s\n", fileName.c_str());

    sanitize();

    std::string json = constructJSON();
    if (json.empty()) {
        return false;
    }

    if (!fileWrite(fileName, json)) {
        return false;
    }

    return true;
}

void Settings::dump()
{
#if !defined(NDEBUG)
    for (const AutoTray & autoTray : autoTrays_) {
        DEBUG_PRINTF("\t%s:\n", settingKeys_[SK_AutoTray]);
        DEBUG_PRINTF("\t\t%s: '%s'\n", settingKeys_[SK_Executable], autoTray.executable_.c_str());
        DEBUG_PRINTF("\t\t%s: '%s'\n", settingKeys_[SK_WindowClass], autoTray.windowClass_.c_str());
        DEBUG_PRINTF("\t\t%s: '%s'\n", settingKeys_[SK_WindowTitle], autoTray.windowTitle_.c_str());
    }
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_.c_str());
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_HotkeyRestore], hotkeyRestore_.c_str());
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_ModifiersOverride], modifiersOverride_.c_str());
    DEBUG_PRINTF("\t%s: %u\n", settingKeys_[SK_PollInterval], pollInterval_);
#endif
}

void Settings::addAutoTray(const std::string & executable, const std::string & windowClass, const std::string & windowTitle)
{
    AutoTray autoTray;
    autoTray.executable_ = executable;
    autoTray.windowClass_ = windowClass;
    autoTray.windowTitle_ = windowTitle;
    autoTrays_.push_back(autoTray);
}

bool Settings::parseJson(const std::string & json)
{
    const cJSON * cjson = cJSON_Parse(json.c_str());
    if (!cjson) {
        DEBUG_PRINTF("failed to parse settings JSON:\n%s\n", cJSON_GetErrorPtr());
        return false;
    }

    DEBUG_PRINTF("parsed settings JSON:\n%s\n", cJSON_Print(cjson));

    const cJSON * autotray = cJSON_GetObjectItemCaseSensitive(cjson, settingKeys_[SK_AutoTray]);
    if (autotray) {
        iterateArray(autotray, autoTrayItemCallback, this);
    }

    hotkeyMinimize_ = getString(cjson, settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_.c_str());
    hotkeyRestore_ = getString(cjson, settingKeys_[SK_HotkeyRestore], hotkeyRestore_.c_str());
    modifiersOverride_ = getString(cjson, settingKeys_[SK_ModifiersOverride], modifiersOverride_.c_str());
    pollInterval_ = (unsigned int)getNumber(cjson, settingKeys_[SK_PollInterval], (double)pollInterval_);

    sanitize();

    return true;
}

std::string Settings::constructJSON()
{
    cJSON * cjson = cJSON_CreateObject();
    if (!cjson) {
        DEBUG_PRINTF("failed to create settings JSON object\n");
        return std::string();
    }

    bool fail = false;

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

                if (!cJSON_AddItemToArray(autotrayArray, item)) {
                    fail = true;
                }
            }
        }
    }

    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_HotkeyRestore], hotkeyRestore_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_ModifiersOverride], modifiersOverride_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddNumberToObject(cjson, settingKeys_[SK_PollInterval], (double)pollInterval_)) {
        fail = true;
    }

    if (fail) {
        DEBUG_PRINTF("Failed to construct json settings\n");
        cJSON_Delete(cjson);
        return std::string();
    }

    return cJSON_Print(cjson);
}

void Settings::sanitize()
{
    for (auto it = autoTrays_.begin(); it != autoTrays_.end(); ++it) {
        AutoTray & autoTray = *it;
        if ((autoTray.executable_.empty()) && (!autoTray.windowClass_.empty()) && (!autoTray.windowTitle_.empty())) {
            DEBUG_PRINTF("Removing empty auto-tray item\n");
            it = autoTrays_.erase(it);
        }
    }
}

bool Settings::autoTrayItemCallback(const cJSON * cjson, void * userData)
{
    if (!cJSON_IsObject(cjson)) {
        DEBUG_PRINTF("bad type for '%s'\n", cjson->string);
        return false;
    }

    const char * executable = getString(cjson, settingKeys_[SK_Executable], nullptr);
    const char * windowClass = getString(cjson, settingKeys_[SK_WindowClass], nullptr);
    const char * windowTitle = getString(cjson, settingKeys_[SK_WindowTitle], nullptr);
    if (executable || windowClass || windowTitle) {
        Settings * settings = (Settings *)userData;
        settings->addAutoTray(executable ? executable : "", windowClass ? windowClass : "", windowTitle ? windowTitle : "");
    }

    return true;
}

// bool getBool(const cJSON * cjson, const char * key, bool defaultValue)
// {
//     const cJSON * item = cJSON_GetObjectItemCaseSensitive(cjson, key);
//     if (!item) {
//         return defaultValue;
//     }
//
//     if (!cJSON_IsBool(item)) {
//         DEBUG_PRINTF("bad type for '%s'\n", item->string);
//         return defaultValue;
//     }
//
//     return cJSON_IsTrue(item) ? true : false;
// }

double getNumber(const cJSON * cjson, const char * key, double defaultValue)
{
    const cJSON * item = cJSON_GetObjectItemCaseSensitive(cjson, key);
    if (!item) {
        return defaultValue;
    }

    if (!cJSON_IsNumber(item)) {
        DEBUG_PRINTF("bad type for '%s'\n", item->string);
        return defaultValue;
    }

    return cJSON_GetNumberValue(item);
}

const char * getString(const cJSON * cjson, const char * key, const char * defaultValue)
{
    cJSON * item = cJSON_GetObjectItemCaseSensitive(cjson, key);
    if (!item) {
        return defaultValue;
    }

    const char * str = cJSON_GetStringValue(item);
    if (!str) {
        DEBUG_PRINTF("bad type for '%s'\n", cjson->string);
        return defaultValue;
    }

    return str;
}

void iterateArray(const cJSON * cjson, bool (*callback)(const cJSON *, void *), void * userData)
{
    if (!cJSON_IsArray(cjson)) {
        DEBUG_PRINTF("bad type for '%s'\n", cjson->string);
    } else {
        int arrSize = cJSON_GetArraySize(cjson);
        for (int i = 0; i < arrSize; ++i) {
            const cJSON * item = cJSON_GetArrayItem(cjson, i);
            if (!callback(item, userData)) {
                break;
            }
        }
    }
}
