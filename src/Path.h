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

// Naming convention
// Dir: directory (C:\Windows)
// FileName: filename (notepad.exe)
// FullPath: Dir + FileName (C:\Windows\notepad.exe)

std::string getAppDataDir();
std::string getExecutableFullPath();
// std::string getExecutableFileName();
std::string getExecutableDir();
std::string getStartupDir();
std::string getWriteableDir();
std::string pathJoin(const std::string & path1, const std::string & path2);
bool createShortcut(const std::string & shortcutFullPath, const std::string & executableFullPath);
