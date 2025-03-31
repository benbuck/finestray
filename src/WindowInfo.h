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

// Windows
#include <Windows.h>

// Standard library
#include <string>

class WindowInfo
{
public:
    WindowInfo() = delete;
    explicit WindowInfo(HWND hwnd);
    ~WindowInfo() = default;

    const std::string & className() const { return className_; }

    const std::string & executable() const { return executable_; }

    const std::string & title() const { return title_; }

private:
    HWND hwnd_ {};
    std::string className_;
    std::string executable_;
    std::string title_;
};
