// Copyright 2020 Benbuck Nason

#include "File.h"

#include "DebugPrint.h"

std::string fileRead(const std::wstring & fileName)
{
    std::string contents;

    HANDLE file = CreateFile(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        DEBUG_PRINTF("could not open '%ws' for reading\n", fileName.c_str());
    } else {
        DWORD fileSize = GetFileSize(file, NULL);
        DWORD bufferSize = fileSize + 1;
        char * buffer = new char[bufferSize];
        buffer[bufferSize - 1] = '\0';
        if (!buffer) {
            DEBUG_PRINTF("could not allocate buffer size %d for reading\n", bufferSize);
        } else {
            DWORD bytesRead = 0;
            if (!ReadFile(file, buffer, fileSize, &bytesRead, NULL)) {
                DEBUG_PRINTF("could not read %d bytes from '%ws'\n", fileSize, fileName.c_str());
                delete[] buffer;
                buffer = nullptr;
            } else {
                if (bytesRead < fileSize) {
                    DEBUG_PRINTF("only read %d bytes from '%ws', expected %d\n", bytesRead, fileName.c_str(), fileSize);
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
        return L"";
    }

    TCHAR * sep = wcsrchr(path, L'\\');
    if (!sep) {
        DEBUG_PRINTF("path has no separator\n");
        return L"";
    }

    size_t pathChars = sep - path;
    std::wstring exePath;
    exePath.reserve(pathChars + 1);
    exePath.resize(pathChars);
    wcsncpy_s(&exePath[0], pathChars + 1, path, pathChars);
    return exePath;
}
