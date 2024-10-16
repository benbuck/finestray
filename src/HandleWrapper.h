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

// App
#include "DebugPrint.h"

// Windows
#include <Windows.h>

class HandleWrapper
{
public:
    HandleWrapper(HANDLE handle)
        : handle_(handle)
    {
    }

    ~HandleWrapper()
    {
        if (handle_ != INVALID_HANDLE_VALUE) {
            if (!CloseHandle((HANDLE)handle_)) {
                DEBUG_PRINTF("failed to close handle: %#x\n", handle_);
            }
        }
    }

    operator HANDLE() const { return handle_; }

    operator bool() const { return handle_ != INVALID_HANDLE_VALUE; }

private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
};
