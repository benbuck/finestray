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

#include "StringUtility.h"

// App
#include "Log.h"

// Windows
#include <Windows.h>

// Standard library
#include <algorithm>

namespace StringUtility
{

std::string toLower(const std::string & s)
{
    std::string lower(s);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](char c) {
        return (char)std::tolower(c);
    });
    return lower;
}

std::string trim(const std::string & s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isspace(*it)) {
        it++;
    }
    std::string::const_reverse_iterator rit = s.rbegin();
    while ((rit.base() != it) && std::isspace(*rit)) {
        rit++;
    }
    return std::string(it, rit.base());
}

std::vector<std::string> split(const std::string & s, const std::string & delimiters)
{
    std::vector<std::string> strings;
    size_t start;
    size_t end = 0;
    while ((start = s.find_first_not_of(delimiters, end)) != std::string::npos) {
        end = s.find_first_of(delimiters, start);
        strings.push_back(s.substr(start, end - start));
    }
    return strings;
}

std::string wideStringToString(const std::wstring & ws)
{
    int ret = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (ret <= 0) {
        WARNING_PRINTF("WideCharToMultiByte() failed: %s\n", lastErrorString().c_str());
        return std::string();
    }

    std::string s;
    s.resize(ret + 1);

    ret = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], (int)s.size(), nullptr, nullptr);
    if (ret <= 0) {
        WARNING_PRINTF("WideCharToMultiByte() failed: %s\n", lastErrorString().c_str());
        return std::string();
    }

    return s;
}

std::wstring stringToWideString(const std::string & s)
{
    int ret = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (ret <= 0) {
        WARNING_PRINTF("MultiByteToWideChar() failed: %s\n", lastErrorString().c_str());
        return std::wstring();
    }

    std::wstring ws;
    ws.resize(ret + 1);

    ret = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], (int)ws.size());
    if (ret <= 0) {
        WARNING_PRINTF("MultiByteToWideChar() failed: %s\n", lastErrorString().c_str());
        return std::wstring();
    }

    return ws;
}

std::string errorToString(unsigned long error)
{
    LPSTR str;
    if (!FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&str,
            0,
            nullptr)) {
        return std::to_string(error);
    }

    std::string ret(str);
    LocalFree(str);
    return ret;
}

std::string lastErrorString()
{
    DWORD lastError = GetLastError();
    return std::to_string(lastError) + " - " + errorToString(lastError);
}

// bool stringToBool(const std::string s, bool & b)
// {
//     if ((s == "1") || (toLower(s) == "true")) {
//         b = true;
//         return true;
//     }
//
//     if ((s == "0") || (toLower(s) == "false")) {
//         b = false;
//         return true;
//     }
//
//     return false;
// }

} // namespace StringUtility
