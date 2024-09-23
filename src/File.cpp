// Copyright 2020 Benbuck Nason

#include "File.h"

// MinTray
#include "DebugPrint.h"
#include "StringUtility.h"

// Standard library
#include <cstring>

std::string fileRead(const std::string & fileName)
{
    std::string contents;

    HANDLE file =
        CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DEBUG_PRINTF(
            "could not open '%s' for reading, CreateFileA() failed: %s\n",
            fileName.c_str(),
            StringUtility::lastErrorString().c_str());
    } else {
        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(file, &fileSize)) {
            DEBUG_PRINTF(
                "could not get file size for '%s', GetFileSizeEx() failed: %s\n",
                fileName.c_str(),
                StringUtility::lastErrorString().c_str());
        } else {
            std::string buffer;
            buffer.resize(fileSize.QuadPart + 1);
            buffer[buffer.size() - 1] = '\0';

            DWORD bytesRead = 0;
            if (!ReadFile(file, &buffer[0], fileSize.LowPart, &bytesRead, nullptr)) {
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

        CloseHandle(file);
    }

    return contents;
}

bool fileWrite(const std::string & fileName, const std::string & contents)
{
    HANDLE file = CreateFileA(
        fileName.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS | TRUNCATE_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DEBUG_PRINTF(
            "could not open '%s' for writing, CreateFileA() failed: %s\n",
            fileName.c_str(),
            StringUtility::lastErrorString().c_str());
        return false;
    } else {
        DWORD bytesWritten = 0;
        if (!WriteFile(file, contents.c_str(), (DWORD)contents.size(), &bytesWritten, nullptr)) {
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

    CloseHandle(file);

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
