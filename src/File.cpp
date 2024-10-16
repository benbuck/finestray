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

#include "File.h"

// App
#include "DebugPrint.h"
#include "StringUtility.h"

// Windows
#include <ShlObj.h>
#include <Shlwapi.h>

// Standard library
#include <cstring>

std::string fileRead(const std::string & fileName)
{
    std::string contents;

    HANDLE hfile =
        CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hfile == INVALID_HANDLE_VALUE) {
        DEBUG_PRINTF(
            "could not open '%s' for reading, CreateFileA() failed: %s\n",
            fileName.c_str(),
            StringUtility::lastErrorString().c_str());
    } else {
        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hfile, &fileSize)) {
            DEBUG_PRINTF(
                "could not get file size for '%s', GetFileSizeEx() failed: %s\n",
                fileName.c_str(),
                StringUtility::lastErrorString().c_str());
        } else {
            std::string buffer;
            buffer.resize(fileSize.QuadPart + 1);
            buffer[buffer.size() - 1] = '\0';

            DWORD bytesRead = 0;
            if (!ReadFile(hfile, &buffer[0], fileSize.LowPart, &bytesRead, nullptr)) {
                DEBUG_PRINTF(
                    "could not read %d bytes from '%s', ReadFile() failed: %s\n",
                    fileSize,
                    fileName.c_str(),
                    GetLastError());
            } else {
                if (bytesRead != fileSize.LowPart) {
                    DEBUG_PRINTF("read %d bytes from '%s', expected %d\n", bytesRead, fileName.c_str(), fileSize);
                } else {
                    contents = buffer;
                }
            }
        }

        CloseHandle(hfile);
    }

    return contents;
}

bool fileWrite(const std::string & fileName, const std::string & contents)
{
    HANDLE hfile =
        CreateFileA(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hfile == INVALID_HANDLE_VALUE) {
        DEBUG_PRINTF(
            "could not open '%s' for writing, CreateFileA() failed: %s\n",
            fileName.c_str(),
            StringUtility::lastErrorString().c_str());
        return false;
    } else {
        DWORD bytesWritten = 0;
        if (!WriteFile(hfile, contents.c_str(), (DWORD)contents.size(), &bytesWritten, nullptr)) {
            DEBUG_PRINTF(
                "could not write %d bytes to '%s', WriteFile() failed: %s\n",
                contents.size(),
                fileName.c_str(),
                GetLastError());
            return false;
        } else {
            if (bytesWritten != contents.size()) {
                DEBUG_PRINTF("wrote %d bytes to '%s', expected %zu\n", bytesWritten, fileName.c_str(), contents.size());
                return false;
            }
        }
    }

    CloseHandle(hfile);

    return true;
}

bool fileExists(const std::string & filename)
{
    DWORD attrib = GetFileAttributesA(filename.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool directoryExists(const std::string & directory)
{
    DWORD attrib = GetFileAttributesA(directory.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool deleteFile(const std::string & fileName)
{
    if (!DeleteFileA(fileName.c_str())) {
        DEBUG_PRINTF(
            "could not delete '%s', DeleteFileA() failed: %s\n",
            fileName.c_str(),
            StringUtility::lastErrorString().c_str());
        return false;
    }

    return true;
}

std::string getExecutablePath()
{
    CHAR path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) <= 0) {
        DEBUG_PRINTF(
            "could not get executable path, GetModuleFileNameA() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return std::string();
    }

    char * sep = strrchr(path, '\\');
    if (!sep) {
        DEBUG_PRINTF("path '%s' has no separator\n", path);
        return std::string();
    }

    size_t pathChars = sep - path;
    std::string exePath;
    exePath.reserve(pathChars + 1);
    exePath.resize(pathChars);
    strncpy_s(&exePath[0], pathChars + 1, path, pathChars);
    return exePath;
}

std::string getAppDataPath()
{
    CHAR path[MAX_PATH];

    HRESULT hresult = SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path);
    if (FAILED(hresult)) {
        DEBUG_PRINTF(
            "could not get app data path, SHGetFolderPath() failed: %s\n",
            StringUtility::errorToString(hresult).c_str());
        return std::string();
    }

    return path;
}

std::string getStartupPath()
{
    CHAR path[MAX_PATH];

    HRESULT hresult = SHGetFolderPathA(nullptr, CSIDL_STARTUP, nullptr, 0, path);
    if (FAILED(hresult)) {
        DEBUG_PRINTF(
            "could not get startup path, SHGetFolderPath() failed: %s\n",
            StringUtility::errorToString(hresult).c_str());
        return std::string();
    }

    return path;
}

std::string pathJoin(const std::string & path1, const std::string & path2)
{
    std::string path;
    path.reserve(max(MAX_PATH, path1.size() + path2.size() + 2));

    LPSTR result = PathCombineA(&path[0], path1.c_str(), path2.c_str());
    if (!result) {
        DEBUG_PRINTF(
            "could not join paths '%s' and '%s', PathCombineA() failed: %s\n",
            path1.c_str(),
            path2.c_str(),
            StringUtility::lastErrorString().c_str());
        return std::string();
    }

    return path;
}

bool createShortcut(const std::string & shortcutPath, const std::string & executablePath)
{
    IShellLinkA * pShellLink = nullptr;
    HRESULT hresult =
        CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (LPVOID *)&pShellLink);
    if (FAILED(hresult)) {
        DEBUG_PRINTF("failed to create shell link: %s\n", StringUtility::errorToString(hresult).c_str());
        return false;
    }

    hresult = pShellLink->SetPath(executablePath.c_str());
    if (FAILED(hresult)) {
        DEBUG_PRINTF("failed to set path: %s\n", StringUtility::errorToString(hresult).c_str());
        pShellLink->Release();
        return false;
    }

    IPersistFile * pPersistFile = nullptr;
    hresult = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
    if (FAILED(hresult)) {
        DEBUG_PRINTF("failed to get persist file: %s\n", StringUtility::errorToString(hresult).c_str());
        pShellLink->Release();
        return false;
    }

    std::wstring shortcutPathW = StringUtility::stringToWideString(shortcutPath);
    hresult = pPersistFile->Save(shortcutPathW.c_str(), TRUE);
    if (FAILED(hresult)) {
        DEBUG_PRINTF("failed to save shortcut: %s\n", StringUtility::errorToString(hresult).c_str());
        pPersistFile->Release();
        pShellLink->Release();
        return false;
    }

    pPersistFile->Release();
    pShellLink->Release();

    return true;
}
