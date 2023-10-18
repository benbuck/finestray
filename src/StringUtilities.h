// Copyright 2020 Benbuck Nason

#pragma once

// standard library
#include <string>
#include <vector>

namespace StringUtilities
{

std::string toLower(const std::string & s);
std::string trim(const std::string & s);
std::vector<std::string> split(const std::string & s, const std::string & delimiters);

// bool stringToBool(const std::string s, bool & b);

} // namespace StringUtilities
