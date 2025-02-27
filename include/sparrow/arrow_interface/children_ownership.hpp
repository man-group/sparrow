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

#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    /*
     * Class for tracking ownership of children of an `ArrowArray` or
     * an `ArrowSchema` allocated by sparrow.
     */
    class children_ownership
    {
    public:

        [[nodiscard]] constexpr std::size_t children_size() const noexcept;
        constexpr void set_child_ownership(std::size_t child, bool ownership);
        template <std::ranges::input_range CHILDREN_OWNERSHIP>
        constexpr void set_children_ownership(const CHILDREN_OWNERSHIP& children_ownership_values);
        [[nodiscard]] constexpr bool has_child_ownership(std::size_t child) const;

        constexpr void resize_children(std::size_t size);

    protected:

        constexpr explicit children_ownership(std::size_t size = 0);

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

    /******************
     * Implementation *
     ******************/

    constexpr children_ownership::children_ownership(std::size_t size)
        : m_children(size, true)
    {
    }

    [[nodiscard]] constexpr std::size_t children_ownership::children_size() const noexcept
    {
        return m_children.size();
    }

    constexpr void children_ownership::set_child_ownership(std::size_t child, bool ownership)
    {
        SPARROW_ASSERT_TRUE(child < m_children.size());
        m_children[child] = ownership;
    }

    template <std::ranges::input_range CHILDREN_OWNERSHIP>
    constexpr void
    children_ownership::set_children_ownership(const CHILDREN_OWNERSHIP& children_ownership_values)
    {
        SPARROW_ASSERT_TRUE(children_ownership_values.size() == m_children.size());
        std::copy(children_ownership_values.begin(), children_ownership_values.end(), m_children.begin());
    }

    [[nodiscard]] constexpr bool children_ownership::has_child_ownership(std::size_t child) const
    {
        SPARROW_ASSERT_TRUE(child < m_children.size());
        return m_children[child];
    }

    constexpr void children_ownership::resize_children(std::size_t size)
    {
        m_children.resize(size);
    }

}  // namespace sparrow
