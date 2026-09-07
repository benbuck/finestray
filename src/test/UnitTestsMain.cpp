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
#include "Log.h"
#include "UnitTests.h"

// doctest
#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_CONFIG_NO_POSIX_SIGNALS
#include <doctest/doctest.h>

namespace Test
{

bool runStartupUnitTests()
{
    doctest::Context ctx;
    // Operate fully in-process: no console needed, results via return value.
    ctx.setOption("no-breaks", true);
    ctx.setOption("no-version", true);

    const int failed = ctx.run();
    if (failed != 0) {
        ERROR_PRINTF("debug startup unit tests FAILED: %d case(s) failed\n", failed);
        return false;
    }

    INFO_PRINTF("debug startup unit tests passed\n");
    return true;
}

} // namespace Test
