// Copyright 2020 Benbuck Nason

#include "StringUtilities.h"

// MinTray
#include "DebugPrint.h"

// windows
#include <Windows.h>

// standard library
#include <algorithm>

namespace StringUtilities
{

std::string toLower(const std::string & s)
{
    std::string lower(s);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](char c) { return (char)std::tolower(c); });
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

std::wstring stringToWideString(const std::string & s)
{
    std::wstring ws;

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (sizeNeeded <= 0) {
        DEBUG_PRINTF("MultiByteToWideChar() failed: %u\n", GetLastError());
    } else {
        ws.resize(sizeNeeded);

        int ret = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], sizeNeeded);
        if (ret <= 0) {
            DEBUG_PRINTF("MultiByteToWideChar() failed: %u\n", GetLastError());
        }
    }

    return ws;
}

std::string wideStringToString(const std::wstring & ws)
{
    std::string s;

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) {
        DEBUG_PRINTF("WideCharToMultiByte() failed: %u\n", GetLastError());
    } else {
        s.resize(sizeNeeded);

        int ret = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &s[0], sizeNeeded, nullptr, nullptr);
        if (ret <= 0) {
            DEBUG_PRINTF("WideCharToMultiByte() failed: %u\n", GetLastError());
        }
    }

    return s;
}

bool stringToBool(const std::string s, bool & b)
{
    if ((s == "1") || (toLower(s) == "true")) {
        b = true;
        return true;
    }

    if ((s == "0") || (toLower(s) == "false")) {
        b = false;
        return true;
    }

    return false;
}

} // namespace StringUtilities
