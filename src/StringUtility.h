// Copyright 2020 Benbuck Nason

#pragma once

// Standard library
#include <string>
#include <vector>

namespace StringUtility
{

std::string toLower(const std::string & s);
std::string trim(const std::string & s);
std::vector<std::string> split(const std::string & s, const std::string & delimiters);
std::string wideStringToString(const wchar_t * ws);
std::string errorToString(unsigned long error);
std::string lastErrorString();

// bool stringToBool(const std::string s, bool & b);

} // namespace StringUtility
