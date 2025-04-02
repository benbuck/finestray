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
#include <combaseapi.h>

class COMLibraryWrapper
{
public:
    explicit COMLibraryWrapper() noexcept { initialized_ = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)); }

    ~COMLibraryWrapper()
    {
        if (initialized_) {
            CoUninitialize();
        }
    }

    COMLibraryWrapper(const COMLibraryWrapper &) = delete;
    COMLibraryWrapper(COMLibraryWrapper &&) = delete;
    COMLibraryWrapper & operator=(const COMLibraryWrapper &) = delete;
    COMLibraryWrapper & operator=(COMLibraryWrapper &&) = delete;

    bool initialized() const noexcept { return initialized_; }

private:
    bool initialized_ {};
};
