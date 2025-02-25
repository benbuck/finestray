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

#include "Path.h"

// App
#include "HandleWrapper.h"
#include "Log.h"
#include "StringUtility.h"

// Windows
#include <ShlObj.h>
#include <Shlwapi.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace
{

std::string writeablePath_;

bool checkWriteablePath(const std::string path);

} // anonymous namespace

std::string getAppDataPath()
{
    CHAR path[MAX_PATH];

    HRESULT hresult = SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path);
    if (FAILED(hresult)) {
        WARNING_PRINTF(
            "could not get app data path, SHGetFolderPath() failed: %s\n",
            StringUtility::errorToString(hresult).c_str());
        return std::string();
    }

    return path;
}

std::string getExecutablePath()
{
    CHAR path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) <= 0) {
        WARNING_PRINTF(
            "could not get executable path, GetModuleFileNameA() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return std::string();
    }

    char * sep = strrchr(path, '\\');
    if (!sep) {
        WARNING_PRINTF("path '%s' has no separator\n", path);
        return std::string();
    }

    size_t pathChars = sep - path;
    std::string exePath;
    exePath.reserve(pathChars + 1);
    exePath.resize(pathChars);
    strncpy_s(&exePath[0], pathChars + 1, path, pathChars);
    return exePath;
}

std::string getStartupPath()
{
    CHAR path[MAX_PATH];

    HRESULT hresult = SHGetFolderPathA(nullptr, CSIDL_STARTUP, nullptr, 0, path);
    if (FAILED(hresult)) {
        WARNING_PRINTF(
            "could not get startup path, SHGetFolderPath() failed: %s\n",
            StringUtility::errorToString(hresult).c_str());
        return std::string();
    }

    return path;
}

std::string pathJoin(const std::string & path1, const std::string & path2)
{
    std::string path;
    path.resize(max(MAX_PATH, path1.size() + path2.size() + 2));

    LPSTR result = PathCombineA(&path[0], path1.c_str(), path2.c_str());
    if (!result) {
        WARNING_PRINTF(
            "could not join paths '%s' and '%s', PathCombineA() failed: %s\n",
            path1.c_str(),
            path2.c_str(),
            StringUtility::lastErrorString().c_str());
        return std::string();
    }

    return path;
}

std::string getWriteablePath()
{
    if (!writeablePath_.empty()) {
        return writeablePath_;
    }

    std::string path;

    path = getExecutablePath();
    if (!path.empty() && checkWriteablePath(path)) {
        DEBUG_PRINTF("using executable path '%s' as writeable path\n", path.c_str());
        writeablePath_ = path;
        return path;
    }

    path = getAppDataPath();
    if (!path.empty() && checkWriteablePath(path)) {
        DEBUG_PRINTF("using app data path '%s' as writeable path\n", path.c_str());
        writeablePath_ = path;
        return path;
    }

    WARNING_PRINTF("no writeable path found\n");
    return std::string();
}

bool createShortcut(const std::string & shortcutPath, const std::string & executablePath)
{
    ComPtr<IShellLinkA> shellLink;
    HRESULT hresult = CoCreateInstance(
        CLSID_ShellLink,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IShellLinkA,
        (LPVOID *)shellLink.ReleaseAndGetAddressOf());
    if (FAILED(hresult)) {
        WARNING_PRINTF("failed to create shell link: %s\n", StringUtility::errorToString(hresult).c_str());
        return false;
    }

    hresult = shellLink->SetPath(executablePath.c_str());
    if (FAILED(hresult)) {
        WARNING_PRINTF("failed to set path: %s\n", StringUtility::errorToString(hresult).c_str());
        return false;
    }

    ComPtr<IPersistFile> persistFile;
    hresult = shellLink->QueryInterface(IID_IPersistFile, (LPVOID *)persistFile.ReleaseAndGetAddressOf());
    if (FAILED(hresult)) {
        WARNING_PRINTF("failed to get persist file: %s\n", StringUtility::errorToString(hresult).c_str());
        return false;
    }

    std::wstring shortcutPathW = StringUtility::stringToWideString(shortcutPath);
    hresult = persistFile->Save(shortcutPathW.c_str(), TRUE);
    if (FAILED(hresult)) {
        WARNING_PRINTF("failed to save shortcut: %s\n", StringUtility::errorToString(hresult).c_str());
        return false;
    }

    return true;
}

namespace
{

bool checkWriteablePath(const std::string path)
{
    std::string fileName = pathJoin(path, "test.tmp");
    HandleWrapper file(CreateFileA(
        fileName.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
        nullptr));
    bool writeable = file != INVALID_HANDLE_VALUE;
    DEBUG_PRINTF("path '%s' is %swriteable\n", path.c_str(), writeable ? "" : "not ");
    return writeable;
}

} // anonymous namespace
