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

#include <memory>

#include "sparrow/c_interface/arrow_array/deleter.hpp"

namespace sparrow
{
    /// Unique pointer to an ArrowArray. Must be used to manage the memory of an ArrowArray.
    using arrow_array_unique_ptr = std::unique_ptr<ArrowArray, arrow_array_custom_deleter_struct>;

    /// Shared pointer to an ArrowArray. Must be used to manage the memory of an ArrowArray.
    class arrow_array_shared_ptr : private std::shared_ptr<ArrowArray>
    {
    public:

        using element_type = ArrowArray;

        arrow_array_shared_ptr()
            : std::shared_ptr<ArrowArray>(nullptr, arrow_array_custom_deleter)
        {
        }

        arrow_array_shared_ptr(arrow_array_unique_ptr&& ptr)
            : std::shared_ptr<ArrowArray>(std::move(ptr).release(), arrow_array_custom_deleter)
        {
        }

        arrow_array_shared_ptr(std::nullptr_t) noexcept
            : std::shared_ptr<ArrowArray>(nullptr, arrow_array_custom_deleter)
        {
        }

        arrow_array_shared_ptr(const arrow_array_shared_ptr& other) noexcept = default;

        arrow_array_shared_ptr(arrow_array_shared_ptr&& other) noexcept = default;

        arrow_array_shared_ptr& operator=(const arrow_array_shared_ptr& other) noexcept = default;

        arrow_array_shared_ptr& operator=(arrow_array_shared_ptr&& other) noexcept = default;

        ~arrow_array_shared_ptr() = default;

        using std::shared_ptr<ArrowArray>::reset;

        using std::shared_ptr<ArrowArray>::swap;

        using std::shared_ptr<ArrowArray>::get;

        using std::shared_ptr<ArrowArray>::operator*;

        using std::shared_ptr<ArrowArray>::operator->;

        using std::shared_ptr<ArrowArray>::use_count;

        using std::shared_ptr<ArrowArray>::operator bool;

        using std::shared_ptr<ArrowArray>::owner_before;

        [[nodiscard]] auto& get_deleter() const noexcept;
    };

    [[nodiscard]] inline auto& arrow_array_shared_ptr::get_deleter() const noexcept
    {
        return *std::get_deleter<void (*)(ArrowArray*)>(*this);
    }
}
