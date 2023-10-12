// Copyright 2020 Benbuck Nason

#include "Settings.h"

#include "DebugPrint.h"
#include "cJSON.h"
#include "File.h"

#include <Windows.h>
#include <shellapi.h>
#include <stdlib.h>

static bool getBool(const cJSON * cjson, const char * key, bool defaultValue);
static double getNumber(const cJSON * cjson, const char * key, double defaultValue);
static const char * getString(const cJSON * cjson, const char * key, const char * defaultValue);
static void iterateArray(const cJSON * cjson, bool (*callback)(const cJSON *, void *), void *);
static bool autoTrayItemCallback(const cJSON * cjson, void * userData);

Settings::Settings() : shouldExit_(false), autoTrays_(), enumWindowsIntervalMs_(500), password_(), trayIcon_(true) {}

Settings::~Settings() {}

bool Settings::readFromFile(const std::wstring & fileName)
{
    std::string json = fileRead(fileName);
    if (json.empty()) {
        return false;
    }

    return parseJson(json);
}

void Settings::parseCommandLine()
{
    int argc;
    LPWSTR * argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    for (int a = 0; a < argc; ++a) {
        if (!wcscmp(argv[a], L"--exit")) {
            shouldExit_ = true;
        } else if (!wcscmp(argv[a], L"--enum-windows-interval-ms")) {
            ++a;
            if (a >= argc) {
                DEBUG_PRINTF("expected argument to --enum-windows-interval-ms missing\n");
            } else {
                enumWindowsIntervalMs_ = _wtoi(argv[a]);
            }
        }
    }
}

bool Settings::parseJson(const std::string & json)
{
    const cJSON * cjson = cJSON_Parse(json.c_str());
    if (!cjson) {
        DEBUG_PRINTF("failed to parse settings JSON: %s\n", json.c_str());
        return false;
    }

    DEBUG_PRINTF("parsed settings JSON: %s\n", cJSON_Print(cjson));

    const cJSON * autotray = cJSON_GetObjectItemCaseSensitive(cjson, "auto-tray");
    if (autotray) {
        iterateArray(autotray, autoTrayItemCallback, this);
    }

    enumWindowsIntervalMs_ = (unsigned int)getNumber(cjson, "enum-windows-interval-ms", (double)enumWindowsIntervalMs_);
    hotkeyMinimize_ = getString(cjson, "hotkey-minimize", hotkeyMinimize_.c_str());
    hotkeyRestore_ = getString(cjson, "hotkey-restore", hotkeyRestore_.c_str());
    password_ = getString(cjson, "password", password_.c_str());
    trayIcon_ = getBool(cjson, "tray-icon", trayIcon_);

    return true;
}

void Settings::addAutoTray(const std::string & className, const std::string & titleRegex)
{
    AutoTray autoTray;
    autoTray.className_ = className;
    autoTray.titleRegex_ = titleRegex;
    autoTrays_.emplace_back(autoTray);
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

bool autoTrayItemCallback(const cJSON * cjson, void * userData)
{
    if (!cJSON_IsObject(cjson)) {
        DEBUG_PRINTF("bad type for '%s'\n", cjson->string);
        return false;
    }

    const char * className = getString(cjson, "class-name", nullptr);
    const char * titleRegex = getString(cjson, "title-regex", nullptr);
    if (className || titleRegex) {
        Settings * settings = (Settings *)userData;
        settings->addAutoTray(className ? className : "", titleRegex ? titleRegex : "");
    }
    return true;
}
