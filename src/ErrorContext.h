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
#include <utility>

class ErrorContext
{
public:
    ErrorContext() = default;

    explicit ErrorContext(unsigned int errorId) noexcept
        : errorId_(errorId)
    {
    }

    ErrorContext(unsigned int errorId, std::string errorString) noexcept
        : errorId_(errorId)
        , errorString_(std::move(errorString))
    {
    }

    // NOLINTNEXTLINE(*-explicit-*)
    operator bool() const noexcept { return (errorId_ != 0) || !errorString_.empty(); }

    [[nodiscard]]
    unsigned int errorId() const noexcept
    {
        return errorId_;
    }

    [[nodiscard]]
    const std::string & errorString() const noexcept
    {
        return errorString_;
    }

private:
    unsigned int errorId_ {};
    std::string errorString_;
};
