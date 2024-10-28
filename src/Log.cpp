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
#include "Log.h"

// Windows
#include <Windows.h>

// Standard library
#include <cstdarg>
#include <cstdio>

namespace
{
const char * levelStrings_[] = { "DEBUG - ", "INFO - ", "WARNING - ", "ERROR - " };
} // anonymous namespace

namespace Log
{

void printf(Level level, const char * fmt, ...)
{
    char buf[1024];

    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len >= (int)sizeof(buf)) {
        __debugbreak();
    }

    print(level, buf);
}

void print(Level level, const char * str)
{
    SYSTEMTIME systemTime;
    memset(&systemTime, 0, sizeof(systemTime));
    GetLocalTime(&systemTime);
    char timeStr[32];
    snprintf(
        timeStr,
        sizeof(timeStr),
        "%02u:%02u:%02u.%03u - ",
        systemTime.wHour,
        systemTime.wMinute,
        systemTime.wSecond,
        systemTime.wMilliseconds);
    OutputDebugStringA(timeStr);

    if ((level >= Level::Debug) && (level <= Level::Error)) {
        OutputDebugStringA(levelStrings_[(unsigned int)level]);
    }

    OutputDebugStringA(str);
}

} // namespace Log
