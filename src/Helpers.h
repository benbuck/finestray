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

// App
#include "ErrorContext.h"

// Windows
#include <Windows.h>

// Standard library
#include <string>

std::string getResourceString(unsigned int id);
std::string getWindowText(HWND hwnd);
std::string getWindowClassName(HWND hwnd);
bool isWindowUserVisible(HWND hwnd);
void errorMessage(unsigned int id);
void errorMessage(const ErrorContext & errorContext);
