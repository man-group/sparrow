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

#include <cstddef>
#include <ranges>
#include <vector>

#include "sparrow/config/config.hpp"

namespace sparrow
{
    /*
     * Class for tracking ownership of the dictionary of an `ArrowArray` or
     * an `ArrowSchema` allocated by sparrow.
     */
    class dictionary_ownership
    {
    public:

        void set_dictionary_ownership(bool ownership) noexcept;
        [[nodiscard]] bool has_dictionary_ownership() const noexcept;

    protected:

        explicit dictionary_ownership(bool ownership) noexcept
            : m_has_ownership(ownership)
        {
        }

    private:

        bool m_has_ownership = true;
    };

    /*
     * Class for tracking ownership of children of an `ArrowArray` or
     * an `ArrowSchema` allocated by sparrow.
     */
    class children_ownership
    {
    public:

        [[nodiscard]] std::size_t children_size() const noexcept;
        void set_child_ownership(std::size_t child, bool ownership);
        template <std::ranges::input_range CHILDREN_OWNERSHIP>
        void set_children_ownership(const CHILDREN_OWNERSHIP& children_ownership_values);
        [[nodiscard]] bool has_child_ownership(std::size_t child) const;

        void resize_children(std::size_t size);

    protected:

        explicit children_ownership(std::size_t size = 0);

        template <std::ranges::input_range CHILDREN_OWNERSHIP>
            requires std::is_same_v<std::ranges::range_value_t<CHILDREN_OWNERSHIP>, bool>
        constexpr explicit children_ownership(const CHILDREN_OWNERSHIP& children_ownership_values)
            : m_children(children_ownership_values.begin(), children_ownership_values.end())
        {
        }

    private:

        using children_owner_list = std::vector<bool>;  // TODO : replace by a dynamic_bitset
        children_owner_list m_children = {};
    };


}  // namespace sparrow
