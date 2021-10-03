// Copyright 2020 Benbuck Nason

#include "File.h"

#include "DebugPrint.h"

const char * fileRead(const WCHAR * fileName)
{
    char * contents = nullptr;

    HANDLE file = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        DEBUG_PRINTF("Could not open '%ls' for reading\n", fileName);
    } else {
        DWORD fileSize = GetFileSize(file, NULL);
        DWORD bufferSize = fileSize + 1;
        char * buffer = new char[bufferSize];
        buffer[bufferSize - 1] = '\0';
        if (!buffer) {
            DEBUG_PRINTF("Could not allocate buffer size %d for reading\n", bufferSize);
        } else {
            DWORD bytesRead = 0;
            if (!ReadFile(file, buffer, fileSize, &bytesRead, NULL)) {
                DEBUG_PRINTF("Could not read %d bytes from '%ls'\n", fileSize, fileName);
                delete[] buffer;
                buffer = nullptr;
            } else {
                if (bytesRead < fileSize) {
                    DEBUG_PRINTF("Only read %d bytes from '%ls', expected %d\n", bytesRead, fileName, fileSize);
                    delete[] buffer;
                    buffer = nullptr;
                } else {
                    contents = buffer;
                }
            }
        }

        CloseHandle(file);
    }

    return contents;
}

std::wstring getExecutablePath()
{
    TCHAR path[MAX_PATH];
    if (GetModuleFileName(NULL, path, MAX_PATH) <= 0) {
        DEBUG_PRINTF("GetModuleFileName() failed\n");
        return nullptr;
    }

    TCHAR * sep = wcsrchr(path, L'\\');
    if (!sep) {
        DEBUG_PRINTF("Path has no separator\n");
        return nullptr;
    }

    *sep = L'\0';

    size_t pathChars = sep - path + 1;
    std::wstring exePath;
    exePath.resize(pathChars);
    wcsncpy_s(&exePath[0], pathChars, path, pathChars - 1);
    return exePath;
}
