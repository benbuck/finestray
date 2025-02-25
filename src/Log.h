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

#define DEBUG_PRINTF(fmt, ...) Log::printf(Log::Level::Debug, fmt, ##__VA_ARGS__)
#define INFO_PRINTF(fmt, ...) Log::printf(Log::Level::Info, fmt, ##__VA_ARGS__)
#define WARNING_PRINTF(fmt, ...) Log::printf(Log::Level::Warning, fmt, ##__VA_ARGS__)
#define ERROR_PRINTF(fmt, ...) Log::printf(Log::Level::Error, fmt, ##__VA_ARGS__)

namespace Log
{

void start(bool enable, const std::string & fileName);

enum class Level
{
    Debug,
    Info,
    Warning,
    Error
};

void printf(Level level, const char * fmt, ...);
void print(Level level, const char * str);

} // namespace Log
