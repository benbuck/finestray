// Copyright 2020 Benbuck Nason

#include "Settings.h"

#include "DebugPrint.h"
#include "cJSON.h"

#include <Windows.h>
#include <shellapi.h>
#include <stdlib.h>

static bool getBool(const cJSON * cjson, const char * key, bool defaultValue);
static const char * getString(const cJSON * cjson, const char * key);
static void iterateArray(const cJSON * cjson, bool (*callback)(const cJSON *, void *), void *);
static bool classnameItemCallback(const cJSON * cjson, void * userData);

Settings::Settings() : shouldExit_(false), autotray_(nullptr), autotraySize_(0) {}

Settings::~Settings()
{
    if (autotray_) {
        for (size_t i = 0; i < autotraySize_; ++i) {
            free(autotray_[i].className_);
        }

        free(autotray_);
    }
}

void Settings::parseCommandLine()
{
    int argc;
    LPWSTR * argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    for (int a = 0; a < argc; ++a) {
        if (!wcscmp(argv[a], L"--exit")) {
            shouldExit_ = true;
        }
    }
}

void Settings::parseJson(const char * json)
{
    const cJSON * cjson = cJSON_Parse(json);
    DEBUG_PRINTF("Parsed settings JSON: %s\n", cJSON_Print(cjson));

    const cJSON * autotray = cJSON_GetObjectItemCaseSensitive(cjson, "autotray");
    if (autotray) {
        iterateArray(autotray, classnameItemCallback, this);
    }
}

void Settings::addAutotray(const char * className)
{
    size_t newAutotraySize = autotraySize_ + 1;
    Autotray * newAutotray = (Autotray *)realloc(autotray_, sizeof(Autotray) * newAutotraySize);
    if (!newAutotray) {
        DEBUG_PRINTF("could not allocate %zu window specs\n", newAutotraySize);
        free(autotray_);
        autotray_ = nullptr;
        autotraySize_ = 0;
        return;
    }

    autotray_ = newAutotray;
    autotraySize_ = newAutotraySize;

    Autotray & autotray = autotray_[autotraySize_];

    size_t len = strlen(className) + 1;
    autotray.className_ = (WCHAR *)malloc(sizeof(WCHAR) * len);
    if (!autotray.className_) {
        DEBUG_PRINTF("could not allocate %zu wchars\n", len);
    } else {
        if (MultiByteToWideChar(CP_UTF8, 0, className, (int)len, autotray.className_, (int)len) <= 0) {
            DEBUG_PRINTF("could not convert class name to wide chars\n");
            free(autotray.className_);
            autotray.className_ = nullptr;
        }
    }
}

#if 0
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
#endif

const char * getString(const cJSON * cjson, const char * key)
{
    cJSON * item = cJSON_GetObjectItemCaseSensitive(cjson, key);
    if (!item) {
        return nullptr;
    }

    const char * str = cJSON_GetStringValue(item);
    if (!str) {
        DEBUG_PRINTF("bad type for '%s'\n", cjson->string);
        return nullptr;
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

bool classnameItemCallback(const cJSON * cjson, void * userData)
{
    if (!cJSON_IsObject(cjson)) {
        DEBUG_PRINTF("bad type for '%s'\n", cjson->string);
        return false;
    }

    const char * str = getString(cjson, "classname");
    if (str) {
        Settings * thisPtr = (Settings *)userData;
        thisPtr->addAutotray(str);
    }
    return false;
}
