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
#include "File.h"
#include "HandleWrapper.h"
#include "Helpers.h"
#include "Log.h"
#include "StringUtility.h"


// Windows
#include <Windows.h>

std::string fileRead(const std::string & fileName)
{
    const HandleWrapper file(
        CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (file == INVALID_HANDLE_VALUE) {
        WARNING_PRINTF(
            "could not open '%s' for reading, CreateFileA() failed: %s\n",
            fileName.c_str(),
            StringUtility::lastErrorString().c_str());
        return {};
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(file, &fileSize)) {
        WARNING_PRINTF(
            "could not get file size for '%s', GetFileSizeEx() failed: %s\n",
            fileName.c_str(),
            StringUtility::lastErrorString().c_str());
        return {};
    }

    std::string buffer;
    buffer.resize(fileSize.QuadPart + 1);
    buffer.at(buffer.size() - 1) = '\0';

    DWORD bytesRead = 0;
    if (!ReadFile(file, buffer.data(), fileSize.LowPart, &bytesRead, nullptr)) {
        WARNING_PRINTF(
            "could not read %d bytes from '%s', ReadFile() failed: %s\n",
            fileSize,
            fileName.c_str(),
            GetLastError());
        return {};
    }

    if (bytesRead != fileSize.LowPart) {
        WARNING_PRINTF("read %d bytes from '%s', expected %d\n", bytesRead, fileName.c_str(), fileSize);
        return {};
    }

    return buffer;
}

bool fileWrite(const std::string & fileName, const std::string & contents)
{
    const HandleWrapper file(
        CreateFileA(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (file == INVALID_HANDLE_VALUE) {
        WARNING_PRINTF(
            "could not open '%s' for writing, CreateFileA() failed: %s\n",
            fileName.c_str(),
            StringUtility::lastErrorString().c_str());
        return false;
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(file, contents.c_str(), narrow_cast<DWORD>(contents.size()), &bytesWritten, nullptr)) {
        WARNING_PRINTF(
            "could not write %d bytes to '%s', WriteFile() failed: %s\n",
            contents.size(),
            fileName.c_str(),
            GetLastError());
        return false;
    }

    if (bytesWritten != contents.size()) {
        WARNING_PRINTF("wrote %d bytes to '%s', expected %zu\n", bytesWritten, fileName.c_str(), contents.size());
        return false;
    }

    return true;
}

bool fileExists(const std::string & fileName) noexcept
{
    const DWORD attrib = GetFileAttributesA(fileName.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool fileDelete(const std::string & fileName)
{
    if (!DeleteFileA(fileName.c_str())) {
        WARNING_PRINTF(
            "could not delete '%s', DeleteFileA() failed: %s\n",
            fileName.c_str(),
            StringUtility::lastErrorString().c_str());
        return false;
    }

    return true;
}

bool directoryExists(const std::string & directory) noexcept
{
    const DWORD attrib = GetFileAttributesA(directory.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
}
