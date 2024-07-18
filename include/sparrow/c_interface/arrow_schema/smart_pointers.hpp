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

#include "sparrow/c_interface/arrow_schema/deleter.hpp"
#include "sparrow/c_interface/c_structures.hpp"


namespace sparrow
{
    /**
     * A unique pointer to an ArrowSchema with a custom deleter.
     * It must be used to manage ArrowSchema objects.
     */
    using arrow_schema_unique_ptr = std::unique_ptr<ArrowSchema, arrow_schema_custom_deleter_struct>;

    /// Shared pointer to an ArrowSchema. Must be used to manage the memory of an ArrowSchema.
    class arrow_schema_shared_ptr : private std::shared_ptr<ArrowSchema>
    {
    public:

        using element_type = ArrowSchema;

        arrow_schema_shared_ptr()
            : std::shared_ptr<ArrowSchema>(nullptr, arrow_schema_custom_deleter)
        {
        }

        arrow_schema_shared_ptr(arrow_schema_shared_ptr&& ptr) noexcept
            : std::shared_ptr<ArrowSchema>(std::move(ptr))
        {
        }

        explicit arrow_schema_shared_ptr(const arrow_schema_shared_ptr& ptr) noexcept
            : std::shared_ptr<ArrowSchema>(ptr)
        {
        }

        explicit arrow_schema_shared_ptr(arrow_schema_unique_ptr&& ptr)
            : std::shared_ptr<ArrowSchema>(std::move(ptr).release(), arrow_schema_custom_deleter)
        {
        }

        explicit arrow_schema_shared_ptr(std::nullptr_t) noexcept
            : std::shared_ptr<ArrowSchema>(nullptr, arrow_schema_custom_deleter)
        {
        }

        arrow_schema_shared_ptr& operator=(arrow_schema_shared_ptr&& ptr) noexcept
        {
            std::shared_ptr<ArrowSchema>::operator=(std::move(ptr));
            return *this;
        }

        arrow_schema_shared_ptr& operator=(const arrow_schema_shared_ptr& ptr) noexcept
        {
            std::shared_ptr<ArrowSchema>::operator=(ptr);
            return *this;
        }

        ~arrow_schema_shared_ptr() = default;

        inline void reset() noexcept
        {
            std::shared_ptr<ArrowSchema>::reset();
        }

        inline void reset(ArrowSchema* ptr) noexcept
        {
            std::shared_ptr<ArrowSchema>::reset(ptr, arrow_schema_custom_deleter);
        }

        inline void swap(arrow_schema_shared_ptr& ptr) noexcept
        {
            std::shared_ptr<ArrowSchema>::swap(ptr);
        }

        [[nodiscard]] inline ArrowSchema* get() const noexcept
        {
            return std::shared_ptr<ArrowSchema>::get();
        }

        [[nodiscard]] inline ArrowSchema& operator*() const noexcept
        {
            return std::shared_ptr<ArrowSchema>::operator*();
        }

        [[nodiscard]] inline ArrowSchema* operator->() const noexcept
        {
            return std::shared_ptr<ArrowSchema>::operator->();
        }

        [[nodiscard]] inline long use_count() const noexcept
        {
            return std::shared_ptr<ArrowSchema>::use_count();
        }

        [[nodiscard]] inline explicit operator bool() const noexcept
        {
            return std::shared_ptr<ArrowSchema>::operator bool();
        }

        [[nodiscard]] inline bool owner_before(const arrow_schema_shared_ptr& ptr) const noexcept
        {
            return std::shared_ptr<ArrowSchema>::owner_before(ptr);
        }

        [[nodiscard]] inline auto& get_deleter() noexcept
        {
            return *std::get_deleter<void (*)(ArrowSchema*)>(*this);
        }
    };

}