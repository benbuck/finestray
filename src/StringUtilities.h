// Copyright 2020 Benbuck Nason

#pragma once

#include <string>
#include <vector>

namespace StringUtilities
{

std::string toLower(const std::string & s);
std::string trim(const std::string & s);
std::vector<std::string> split(const std::string & s, const std::string & delimiters);

std::wstring stringToWideString(const std::string & s);
std::string wideStringToString(const std::wstring & ws);

bool stringToBool(const std::string s, bool & b);

} // namespace StringUtilities
