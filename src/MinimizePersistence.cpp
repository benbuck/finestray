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
#include "MinimizePersistence.h"
#include "Log.h"

bool minimizePersistenceValid(MinimizePersistence minimizePersistence) noexcept
{
    switch (minimizePersistence) {
        case MinimizePersistence::Never:
        case MinimizePersistence::Always: {
            return true;
        }

        case MinimizePersistence::None:
        default: {
            WARNING_PRINTF("error, bad minimize persistence: %d\n", minimizePersistence);
            return false;
        }
    }
}

const char * minimizePersistenceToCString(MinimizePersistence minimizePersistence) noexcept
{
    switch (minimizePersistence) {
        case MinimizePersistence::None: return "none";
        case MinimizePersistence::Never: return "never";
        case MinimizePersistence::Always: return "always";

        default: {
            WARNING_PRINTF("error, bad minimize persistence: %d\n", minimizePersistence);
            return "none";
        }
    }
}

MinimizePersistence minimizePersistenceFromCString(const char * minimizePersistenceString) noexcept
{
    if (!strcmp(minimizePersistenceString, "none")) {
        return MinimizePersistence::None;
    }

    if (!strcmp(minimizePersistenceString, "never")) {
        return MinimizePersistence::Never;
    }

    if (!strcmp(minimizePersistenceString, "always")) {
        return MinimizePersistence::Always;
    }

    WARNING_PRINTF("error, bad minimize persistence string: '%s'\n", minimizePersistenceString);
    return MinimizePersistence::None;
}
