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
#include "HandleWrapper.h"
#include "Path.h"
#include "StringUtility.h"

// Windows
#include <Windows.h>

// Standard library
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <vector>

namespace
{

bool started_ = false;
bool enableLogging_ = false;
HandleWrapper fileHandle_;
std::vector<std::string> pendingLogs_;

} // anonymous namespace

namespace Log
{

void start(bool enable, const std::string & fileName)
{
    started_ = true;

    if (!enable || fileName.empty()) {
        enableLogging_ = false;
        fileHandle_.close();
        pendingLogs_.clear();
        return;
    }

    if (fileHandle_ != INVALID_HANDLE_VALUE) {
        WARNING_PRINTF("logging already started\n");
        enableLogging_ = true;
        assert(pendingLogs_.empty());
        return;
    }

    std::string writeableDir = getWriteableDir();
    if (writeableDir.empty()) {
        WARNING_PRINTF("no writeable dir found, logging to file disabled\n");
        enableLogging_ = false;
        pendingLogs_.clear();
        return;
    }

    std::string logFileFullPath = pathJoin(writeableDir, fileName);

    HandleWrapper fileHandle(CreateFileA(
        logFileFullPath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr));

    if (fileHandle == INVALID_HANDLE_VALUE) {
        WARNING_PRINTF(
            "could not open log file '%s' for writing, CreateFileA() failed: %s\n",
            logFileFullPath.c_str(),
            StringUtility::lastErrorString().c_str());
        enableLogging_ = false;
        pendingLogs_.clear();
        return;
    }

    DEBUG_PRINTF("logging to file '%s'\n", logFileFullPath.c_str());
    fileHandle_ = std::move(fileHandle);
    assert(fileHandle_ != INVALID_HANDLE_VALUE);
    enableLogging_ = true;

    for (size_t p = 0; p < pendingLogs_.size(); ++p) {
        const std::string & pendingLog = pendingLogs_[p];
        DWORD bytesWritten = 0;
        WriteFile(fileHandle_, pendingLog.c_str(), (DWORD)pendingLog.size(), &bytesWritten, nullptr);
        assert(bytesWritten == pendingLog.size());
    }
    pendingLogs_.clear();
}

void printf(Level level, const char * fmt, ...)
{
    char fixedBuffer[1024];
    char * buffer = fixedBuffer;
    size_t bufferSize = sizeof(fixedBuffer);

    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buffer, bufferSize, fmt, ap);
    if (len >= (int)bufferSize) {
        bufferSize = (size_t)len + 1;
        buffer = new char[bufferSize];
        len = vsnprintf(buffer, bufferSize, fmt, ap);
        assert(len < (int)bufferSize);
    }
    va_end(ap);

    print(level, buffer);

    if (buffer != fixedBuffer) {
        delete[] buffer;
    }
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
        "%02u:%02u:%02u.%03u",
        systemTime.wHour,
        systemTime.wMinute,
        systemTime.wSecond,
        systemTime.wMilliseconds);

    const char * levelString;
    switch (level) {
        case Level::Debug: levelString = "DEBUG  "; break;
        case Level::Info: levelString = "INFO   "; break;
        case Level::Warning: levelString = "WARNING"; break;
        case Level::Error: levelString = "ERROR  "; break;
        default: {
            levelString = "UNKNOWN";
            assert(false);
            break;
        }
    }

    std::string line = std::string(timeStr) + " - " + levelString + " - " + str;

    OutputDebugStringA(line.c_str());

    if (!started_) {
        pendingLogs_.push_back(line);
        return;
    }

    if (enableLogging_ && (fileHandle_ != INVALID_HANDLE_VALUE)) {
        DWORD bytesWritten = 0;
        WriteFile(fileHandle_, line.c_str(), (DWORD)line.size(), &bytesWritten, nullptr);
        assert(bytesWritten == line.size());
    }

    // #if defined(_DEBUG)
    //     if ((level == Level::Error) && IsDebuggerPresent()) {
    //         __debugbreak();
    //     }
    // #endif
}

} // namespace Log
