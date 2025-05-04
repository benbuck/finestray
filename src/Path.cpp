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
#include "Path.h"
#include "AppName.h"
#include "File.h"
#include "HandleWrapper.h"
#include "Log.h"
#include "StringUtility.h"

// Windows
#include <ShlObj.h>
#include <Shlwapi.h>
#include <wrl/client.h>

// Standard library
#include <algorithm>

using Microsoft::WRL::ComPtr;

namespace
{

std::string appDataDir_;
std::string startupDir_;
std::string writeableDir_;
std::string executableFullPath_;
std::string executableFileName_;
std::string executableDir_;

bool getExecutablePathComponents();
bool checkWriteableDir(const std::string & dir);

} // anonymous namespace

std::string getAppDataDir()
{
    if (!appDataDir_.empty()) {
        return appDataDir_;
    }

    CHAR dir[MAX_PATH] = {};

    HRESULT const hresult = SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, dir);
    if (FAILED(hresult)) {
        WARNING_PRINTF(
            "could not get app data dir, SHGetFolderPath() failed: %s\n",
            StringUtility::errorToString(HRESULT_CODE(hresult)).c_str());
        return {};
    }

    appDataDir_ = dir;
    return appDataDir_;
}

std::string getExecutableFullPath()
{
    if (executableFullPath_.empty() && !getExecutablePathComponents()) {
        return {};
    }

    return executableFullPath_;
}

// std::string getExecutableFileName()
//{
//     if (executableFileName_.empty() && !getExecutablePathComponents()) {
//         return {};
//     }
//
//     return executableFileName_;
// }

std::string getExecutableDir()
{
    if (executableDir_.empty() && !getExecutablePathComponents()) {
        return {};
    }

    return executableDir_;
}

std::string getStartupDir()
{
    if (!startupDir_.empty()) {
        return startupDir_;
    }

    CHAR dir[MAX_PATH] = {};

    HRESULT const hresult = SHGetFolderPathA(nullptr, CSIDL_STARTUP, nullptr, 0, dir);
    if (FAILED(hresult)) {
        WARNING_PRINTF(
            "could not get startup dir, SHGetFolderPath() failed: %s\n",
            StringUtility::errorToString(HRESULT_CODE(hresult)).c_str());
        return {};
    }

    startupDir_ = dir;
    return startupDir_;
}

std::string pathJoin(const std::string & path1, const std::string & path2)
{
    if (path1.empty()) {
        return path2;
    }
    if (path2.empty()) {
        return path1;
    }

    size_t pathSize = std::min<size_t>(MAX_PATH, path1.size() + path2.size() + 2);

    std::string path;
    path.resize(pathSize);

    LPCSTR result = PathCombineA(path.data(), path1.c_str(), path2.c_str());
    if (!result) {
        WARNING_PRINTF(
            "could not join paths '%s' and '%s', PathCombineA() failed: %s\n",
            path1.c_str(),
            path2.c_str(),
            StringUtility::lastErrorString().c_str());
        return {};
    }

    pathSize = strlen(result);
    path.resize(pathSize);

    return path;
}

std::string getWriteableDir()
{
    if (!writeableDir_.empty()) {
        return writeableDir_;
    }

    std::string dir;

    dir = getAppDataDir();
    if (!dir.empty()) {
        dir = pathJoin(dir, APP_NAME);
        if (!directoryExists(dir)) {
            if (CreateDirectoryA(dir.c_str(), nullptr)) {
                DEBUG_PRINTF("created app data dir '%s'\n", dir.c_str());
            } else {
                WARNING_PRINTF(
                    "could not create directory '%s', CreateDirectoryA() failed: %s\n",
                    dir.c_str(),
                    StringUtility::lastErrorString().c_str());
            }
        }
        if (directoryExists(dir) && checkWriteableDir(dir)) {
            DEBUG_PRINTF("using app data dir '%s' as writeable dir\n", dir.c_str());
            writeableDir_ = dir;
            return dir;
        }
    }

    dir = getExecutableDir();
    if (!dir.empty() && checkWriteableDir(dir)) {
        DEBUG_PRINTF("using executable dir '%s' as writeable dir\n", dir.c_str());
        writeableDir_ = dir;
        return dir;
    }

    WARNING_PRINTF("no writeable dir found\n");
    return {};
}

bool createShortcut(const std::string & shortcutFullPath, const std::string & executableFullPath)
{
    ComPtr<IShellLinkA> shellLink;
    HRESULT hresult = CoCreateInstance(
        CLSID_ShellLink,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IShellLinkA,
        reinterpret_cast<LPVOID *>(shellLink.ReleaseAndGetAddressOf()));
    if (FAILED(hresult)) {
        WARNING_PRINTF("failed to create shell link: %s\n", StringUtility::errorToString(HRESULT_CODE(hresult)).c_str());
        return false;
    }

    hresult = shellLink->SetPath(executableFullPath.c_str());
    if (FAILED(hresult)) {
        WARNING_PRINTF("failed to set path: %s\n", StringUtility::errorToString(HRESULT_CODE(hresult)).c_str());
        return false;
    }

    ComPtr<IPersistFile> persistFile;
    hresult =
        shellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID *>(persistFile.ReleaseAndGetAddressOf()));
    if (FAILED(hresult)) {
        WARNING_PRINTF("failed to get persist file: %s\n", StringUtility::errorToString(HRESULT_CODE(hresult)).c_str());
        return false;
    }

    const std::wstring shortcutPathW = StringUtility::stringToWideString(shortcutFullPath);
    hresult = persistFile->Save(shortcutPathW.c_str(), TRUE);
    if (FAILED(hresult)) {
        WARNING_PRINTF("failed to save shortcut: %s\n", StringUtility::errorToString(HRESULT_CODE(hresult)).c_str());
        return false;
    }

    return true;
}

namespace
{

bool getExecutablePathComponents()
{
    CHAR moduleFullPath[256];

    const size_t length = GetModuleFileNameA(nullptr, moduleFullPath, sizeof(moduleFullPath));
    if (length == 0) {
        WARNING_PRINTF(
            "could not get executable full path, GetModuleFileNameA() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    if (length >= sizeof(moduleFullPath)) {
        WARNING_PRINTF("executable full path too long\n");
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            ERROR_PRINTF("GetModuleFileNameA() failed: %s\n", StringUtility::lastErrorString().c_str());
        }
        return false;
    }

    if (length != strlen(moduleFullPath)) {
        WARNING_PRINTF("unexpected length mismatch, GetModuleFileNameA %zu, strlen %zu\n", length, strlen(moduleFullPath));
    }

    executableFullPath_ = moduleFullPath;
    DEBUG_PRINTF("executable full path: %s\n", executableFullPath_.c_str());

    LPCSTR fileName = PathFindFileNameA(moduleFullPath);
    if (fileName == moduleFullPath) {
        WARNING_PRINTF(
            "could not find file name in module full path '%s', PathFindFileNameA() failed: %s\n",
            moduleFullPath,
            StringUtility::lastErrorString().c_str());
        return false;
    }

    executableFileName_ = fileName;
    DEBUG_PRINTF("executable file name: %s\n", executableFileName_.c_str());

    if (!PathRemoveFileSpecA(moduleFullPath)) {
        WARNING_PRINTF(
            "could not remove file spec from module full path '%s', PathRemoveFileSpecA() failed: %s\n",
            moduleFullPath,
            StringUtility::lastErrorString().c_str());
        return false;
    }

    executableDir_ = moduleFullPath;
    DEBUG_PRINTF("executable dir: %s\n", executableDir_.c_str());

    return true;
}

bool checkWriteableDir(const std::string & dir)
{
    const std::string fullPath = pathJoin(dir, "test.tmp");
    const HandleWrapper file(CreateFileA(
        fullPath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
        nullptr));
    const bool writeable = file != INVALID_HANDLE_VALUE;
    DEBUG_PRINTF("dir '%s' %s writeable\n", dir.c_str(), writeable ? "is" : "is not");
    return writeable;
}

} // anonymous namespace
