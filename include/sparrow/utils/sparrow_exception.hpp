// Copyright 2024 Man Group Operations Limited
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
#include <stdexcept>
#include <string>

#include "sparrow/config/config.hpp"

namespace sparrow
{
// MSVC warns C4275 when a dllexport class inherits from a non-dllexport base.
// This is safe and expected for exception classes; suppress the warning.
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4275)
#endif
    class SPARROW_API contract_assertion_error : public std::runtime_error
    {
    public:

        explicit contract_assertion_error(const std::string& message);
        explicit contract_assertion_error(const char* message);
    };
#ifdef _MSC_VER
#    pragma warning(pop)
#endif
}
