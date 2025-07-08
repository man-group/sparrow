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
//

#include "sparrow/arrow_interface/private_data_ownership.hpp"

#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    void dictionary_ownership::set_dictionary_ownership(bool ownership) noexcept
    {
        m_has_ownership = ownership;
    }

    [[nodiscard]] bool dictionary_ownership::has_dictionary_ownership() const noexcept
    {
        return m_has_ownership;
    }

    children_ownership::children_ownership(std::size_t size)
        : m_children(size, true)
    {
    }

    [[nodiscard]] std::size_t children_ownership::children_size() const noexcept
    {
        return m_children.size();
    }

    void children_ownership::set_child_ownership(std::size_t child, bool ownership)
    {
        SPARROW_ASSERT_TRUE(child < m_children.size());
        m_children[child] = ownership;
    }

    template <std::ranges::input_range CHILDREN_OWNERSHIP>
    void children_ownership::set_children_ownership(const CHILDREN_OWNERSHIP& children_ownership_values)
    {
        SPARROW_ASSERT_TRUE(children_ownership_values.size() == m_children.size());
        std::copy(children_ownership_values.begin(), children_ownership_values.end(), m_children.begin());
    }

    [[nodiscard]] bool children_ownership::has_child_ownership(std::size_t child) const
    {
        SPARROW_ASSERT_TRUE(child < m_children.size());
        return m_children[child];
    }

    void children_ownership::resize_children(std::size_t size)
    {
        m_children.resize(size);
    }
}  // namespace sparrow
