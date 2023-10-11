// Copyright 2020 Benbuck Nason

#include "StringUtilities.h"

#include <algorithm>

namespace StringUtilities
{

#if 0 // !defined(NDEBUG)
static std::string ws2s(const std::wstring & wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}
#endif

std::string tolower(const std::string & s)
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

    // std::vector<std::string> tokens;
    // size_t startIndex = 0;
    // do {
    //     size_t endIndex = s.find_first_of(delimiters, startIndex);
    //     if (endIndex != std::string::npos) {
    //         std::string token(&s[startIndex], &s[endIndex]);
    //     }
    //     startIndex = endIndex;
    // } while (startIndex < s.size());

    // std::string::const_iterator start = std::cbegin(s);
    // while (start != std::cend(s)) {
    //     std::string::const_iterator finish = std::find_first_of(start, std::cend(s), delimiters);
    //     tokens.push_back(s);
    //     start = finish;
    //     if (start != std::cend(s)) {
    //         ++start;
    //     }
}

} // namespace StringUtilities
