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

// The file and directory paths here may be relative or absolute

std::string fileRead(const std::string & fileName);
bool fileWrite(const std::string & fileName, const std::string & contents);
bool fileExists(const std::string & fileName) noexcept;
bool fileDelete(const std::string & fileName);
bool directoryExists(const std::string & directory) noexcept;
