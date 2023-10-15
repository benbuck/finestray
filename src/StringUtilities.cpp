// Copyright 2020 Benbuck Nason

#include "StringUtilities.h"

// standard library
#include <algorithm>

namespace StringUtilities
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
