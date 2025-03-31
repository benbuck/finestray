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
        return static_cast<char>(std::tolower(c));
    });
    return lower;
}

std::string trim(const std::string & s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isspace(*it)) {
        ++it;
    }
    std::string::const_reverse_iterator rit = s.rbegin();
    while ((rit.base() != it) && std::isspace(*rit)) {
        ++rit;
    }
    return { it, rit.base() };
}

std::vector<std::string> split(const std::string & s, const std::string & delimiters)
{
    std::vector<std::string> strings;
    size_t start = 0;
    size_t end = 0;
    while ((start = s.find_first_not_of(delimiters, end)) != std::string::npos) {
        end = s.find_first_of(delimiters, start);
        strings.push_back(s.substr(start, end - start));
    }
    return strings;
}

std::string join(const std::vector<std::string> & vs, const std::string & delimiter)
{
    std::string s;
    for (size_t i = 0; i < vs.size(); ++i) {
        if (i > 0) {
            s += delimiter;
        }
        s += vs.at(i);
    }
    return s;
}

std::string wideStringToString(const std::wstring & ws)
{
    int ret = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (ret <= 0) {
        WARNING_PRINTF("WideCharToMultiByte() failed: %s\n", lastErrorString().c_str());
        return {};
    }

    std::string s;
    s.resize(ret);

    ret = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, s.data(), static_cast<int>(s.size()), nullptr, nullptr);
    if (ret <= 0) {
        WARNING_PRINTF("WideCharToMultiByte() failed: %s\n", lastErrorString().c_str());
        return {};
    }

    s.resize(static_cast<size_t>(ret) - 1); // remove nul terminator

    return s;
}

std::wstring stringToWideString(const std::string & s)
{
    int ret = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (ret <= 0) {
        WARNING_PRINTF("MultiByteToWideChar() failed: %s\n", lastErrorString().c_str());
        return {};
    }

    std::wstring ws;
    ws.resize(ret);

    ret = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), static_cast<int>(ws.size()));
    if (ret <= 0) {
        WARNING_PRINTF("MultiByteToWideChar() failed: %s\n", lastErrorString().c_str());
        return {};
    }

    ws.resize(static_cast<size_t>(ret) - 1); // remove nul terminator

    return ws;
}

std::string errorToString(unsigned int error)
{
    LPSTR str = nullptr;
    const size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&str),
        0,
        nullptr);
    if (!size) {
        return std::to_string(error);
    }

    std::string ret(str, size);
    LocalFree(str);

    // remove trailing newline
    if ((ret.size() > 1) && (ret.at(ret.size() - 1) == '\n')) {
        if ((ret.size() > 2) && (ret.at(ret.size() - 2) == '\r')) {
            ret.resize(ret.size() - 2);
        } else {
            ret.resize(ret.size() - 1);
        }
    }

    return ret;
}

std::string lastErrorString()
{
    DWORD const lastError = GetLastError();
    return std::to_string(lastError) + " - " + errorToString(lastError);
}

} // namespace StringUtility
