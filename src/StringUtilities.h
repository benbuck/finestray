// Copyright 2020 Benbuck Nason

#pragma once

#include <string>
#include <vector>

namespace StringUtilities
{

std::string tolower(const std::string & s);
std::string trim(const std::string & s);
std::vector<std::string> split(const std::string & s, const std::string & delimiters);

} // namespace StringUtilities
