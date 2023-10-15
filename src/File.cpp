// Copyright 2020 Benbuck Nason

#include "File.h"

// MinTray
#include "DebugPrint.h"

// standard library
#include <cstring>

std::string fileRead(const std::string & fileName)
{
    std::string contents;

    HANDLE file =
        CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DEBUG_PRINTF("could not open '%s' for reading, CreateFileA() failed: %u\n", fileName.c_str(), GetLastError());
    } else {
        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(file, &fileSize)) {
            DEBUG_PRINTF("could not get file size for '%s', GetFileSizeEx() failed: %u\n", fileName.c_str(), GetLastError());
        } else {
            std::string buffer;
            buffer.resize(fileSize.QuadPart + 1);
            buffer[buffer.size() - 1] = '\0';

            DWORD bytesRead = 0;
            if (!ReadFile(file, &buffer[0], fileSize.LowPart, &bytesRead, nullptr)) {
                DEBUG_PRINTF(
                    "could not read %d bytes from '%s', ReadFile() failed: %u\n",
                    fileSize,
                    fileName.c_str(),
                    GetLastError());
            } else {
                if (bytesRead < fileSize.LowPart) {
                    DEBUG_PRINTF("only read %d bytes from '%s', expected %d\n", bytesRead, fileName.c_str(), fileSize);
                } else {
                    contents = buffer;
                }
            }
        }

        CloseHandle(file);
    }

    return contents;
}

std::string getExecutablePath()
{
    CHAR path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) <= 0) {
        DEBUG_PRINTF("could not get executable path, GetModuleFileNameA() failed: %u\n", GetLastError());
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
