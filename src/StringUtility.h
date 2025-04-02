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

#pragma once

// Standard library
#include <string>
#include <vector>

namespace StringUtility
{

inline const char * boolToCString(bool b) noexcept
{
    return b ? "true" : "false";
}

std::string toLower(const std::string & s);
std::string trim(const std::string & s);
std::vector<std::string> split(const std::string & s, const std::string & delimiters);
std::string join(const std::vector<std::string> & vs, const std::string & delimiter);
std::string wideStringToString(const std::wstring & ws);
std::wstring stringToWideString(const std::string & s);
std::string errorToString(unsigned int error);
std::string lastErrorString();

// bool stringToBool(const std::string s, bool & b);

} // namespace StringUtility
