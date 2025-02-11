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

class CommandLine
{
public:
    CommandLine();

    bool parse();

    inline int getArgc() const { return static_cast<int>(args_.size()); }

    inline const char ** getArgv() const { return const_cast<const char **>(argv_.data()); }

private:
    std::vector<std::string> args_;
    std::vector<const char *> argv_;
};
