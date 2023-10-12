// Copyright 2020 Benbuck Nason

#include "Settings.h"

#include "DebugPrint.h"
#include "File.h"
#include "StringUtilities.h"

#include <argh.h>

#include <cJSON.h>

#include <Windows.h>
#include <shellapi.h>
#include <stdlib.h>

static bool getBool(const cJSON * cjson, const char * key, bool defaultValue);
static double getNumber(const cJSON * cjson, const char * key, double defaultValue);
static const char * getString(const cJSON * cjson, const char * key, const char * defaultValue);
static void iterateArray(const cJSON * cjson, bool (*callback)(const cJSON *, void *), void *);

enum SettingKeys : unsigned int
{
    SK_AutoTray,
    SK_ClassName,
    SK_TitleRegex,
    SK_HotkeyMinimize,
    SK_HotkeyRestore,
    SK_PollMillis,
    SK_TrayIcon,

    SK_Count
};

static const char * settingKeys_[SK_Count] = { "auto-tray",      "class-name",  "title-regex", "hotkey-minimize",
                                               "hotkey-restore", "poll-millis", "tray-icon" };

Settings::Settings() : autoTrays_(), pollMillis_(500), trayIcon_(true) {}

Settings::~Settings() {}

bool Settings::readFromFile(const std::wstring & fileName)
{
    std::string json = fileRead(fileName);
    if (json.empty()) {
        return false;
    }

    return parseJson(json);
}

bool Settings::parseCommandLine(int argc, char const * const * argv)
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
    // syntax to support the various selector strings would be ugly, inconvenient, and non-intuitive

    argh::string_stream hotkeyMinimizeArg = args(settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_);
    if (hotkeyMinimizeArg && !(hotkeyMinimizeArg >> hotkeyMinimize_)) {
        DEBUG_PRINTF("error, bad %s argument: %s\n", settingKeys_[SK_HotkeyMinimize], hotkeyMinimizeArg.str().c_str());
    }

    argh::string_stream hotkeyRestoreArg = args(settingKeys_[SK_HotkeyRestore], hotkeyRestore_);
    if (hotkeyRestoreArg && !(hotkeyRestoreArg >> hotkeyRestore_)) {
        DEBUG_PRINTF("error, bad %s argument: %s\n", settingKeys_[SK_HotkeyRestore], hotkeyRestoreArg.str().c_str());
    }

    argh::string_stream pollMillisArg = args(settingKeys_[SK_PollMillis], pollMillis_);
    if (pollMillisArg && !(pollMillisArg >> pollMillis_)) {
        DEBUG_PRINTF("error, bad %s argument: %s\n", settingKeys_[SK_PollMillis], pollMillisArg.str().c_str());
    }

    argh::string_stream trayIconArg = args(settingKeys_[SK_TrayIcon], trayIcon_);
    if (trayIconArg && !StringUtilities::stringToBool(trayIconArg.str(), trayIcon_)) {
        DEBUG_PRINTF("error, bad %s argument: %s\n", settingKeys_[SK_TrayIcon], trayIconArg.str().c_str());
    }

    return true;
}

bool Settings::parseJson(const std::string & json)
{
    const cJSON * cjson = cJSON_Parse(json.c_str());
    if (!cjson) {
        DEBUG_PRINTF("failed to parse settings JSON: %s\n", json.c_str());
        return false;
    }

    DEBUG_PRINTF("parsed settings JSON: %s\n", cJSON_Print(cjson));

    const cJSON * autotray = cJSON_GetObjectItemCaseSensitive(cjson, settingKeys_[SK_AutoTray]);
    if (autotray) {
        iterateArray(autotray, autoTrayItemCallback, this);
    }

    hotkeyMinimize_ = getString(cjson, settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_.c_str());
    hotkeyRestore_ = getString(cjson, settingKeys_[SK_HotkeyRestore], hotkeyRestore_.c_str());
    pollMillis_ = (unsigned int)getNumber(cjson, settingKeys_[SK_PollMillis], (double)pollMillis_);
    trayIcon_ = getBool(cjson, settingKeys_[SK_TrayIcon], trayIcon_);

    return true;
}

void Settings::addAutoTray(const std::string & className, const std::string & titleRegex)
{
    AutoTray autoTray;
    autoTray.className_ = className;
    autoTray.titleRegex_ = titleRegex;
    autoTrays_.emplace_back(autoTray);
}

bool Settings::autoTrayItemCallback(const cJSON * cjson, void * userData)
{
    if (!cJSON_IsObject(cjson)) {
        DEBUG_PRINTF("bad type for '%s'\n", cjson->string);
        return false;
    }

    const char * className = getString(cjson, settingKeys_[SK_ClassName], nullptr);
    const char * titleRegex = getString(cjson, settingKeys_[SK_TitleRegex], nullptr);
    if (className || titleRegex) {
        Settings * settings = (Settings *)userData;
        settings->addAutoTray(className ? className : "", titleRegex ? titleRegex : "");
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
        DEBUG_PRINTF("bad type for '%s'\n", item->string);
        return defaultValue;
    }

    return cJSON_IsTrue(item) ? true : false;
}

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
