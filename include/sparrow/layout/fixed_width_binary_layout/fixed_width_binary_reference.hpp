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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <ranges>
#include <string>
#include <vector>

#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"

#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

namespace sparrow
{
    /**
     * Implementation of reference to inner type used for layout L
     *
     * @tparam L the layout type
     */
    template <class L>
    class fixed_width_binary_reference
    {
    public:

        using self_type = fixed_width_binary_reference<L>;
        using value_type = typename L::inner_value_type;
        using reference = typename L::inner_reference;
        using const_reference = typename L::inner_const_reference;
        using size_type = typename L::size_type;
        using difference_type = std::ptrdiff_t;
        using iterator = typename L::data_iterator;
        using const_iterator = typename L::const_data_iterator;

        fixed_width_binary_reference(L* layout, size_type index);
        fixed_width_binary_reference(const fixed_width_binary_reference&) = default;
        fixed_width_binary_reference(fixed_width_binary_reference&&) = default;

        template <std::ranges::sized_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        self_type& operator=(T&& rhs);

        size_type size() const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;
        const_iterator cbegin() const;
        const_iterator cend() const;

        template <std::ranges::input_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        bool operator==(const T& rhs) const;

        template <std::ranges::input_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        auto operator<=>(const T& rhs) const;

    private:

        size_type offset(size_type index) const;

        L* p_layout = nullptr;
        size_type m_index = size_type(0);
    };
}

namespace std
{
    template <typename Layout, template <typename> typename TQual, template <typename> typename UQual>
    struct basic_common_reference<sparrow::fixed_width_binary_reference<Layout>, std::vector<sparrow::byte_t>, TQual, UQual>
    {
        using type = std::vector<sparrow::byte_t>;
    };

    template <typename Layout, template <typename> typename TQual, template <class> class UQual>
    struct basic_common_reference<std::vector<sparrow::byte_t>, sparrow::fixed_width_binary_reference<Layout>, TQual, UQual>
    {
        using type = std::vector<sparrow::byte_t>;
    };
}

namespace sparrow
{
    /***********************************************
     * fixed_width_binary_reference implementation *
     ***********************************************/

    template <class L>
    fixed_width_binary_reference<L>::fixed_width_binary_reference(L* layout, size_type index)
        : p_layout(layout)
        , m_index(index)
    {
    }

    template <class L>
    template <std::ranges::sized_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    auto fixed_width_binary_reference<L>::operator=(T&& rhs) -> self_type&
    {
        SPARROW_ASSERT_TRUE(p_layout->m_element_size == std::ranges::size(rhs));
        p_layout->assign(std::forward<T>(rhs), m_index);
        p_layout->get_arrow_proxy().update_buffers();
        return *this;
    }

    template <class L>
    auto fixed_width_binary_reference<L>::size() const -> size_type
    {
        return p_layout->m_element_size;
    }

    template <class L>
    auto fixed_width_binary_reference<L>::begin() -> iterator
    {
        return iterator(p_layout->data(offset(m_index)));
    }

    template <class L>
    auto fixed_width_binary_reference<L>::end() -> iterator
    {
        return iterator(p_layout->data(offset(m_index + 1)));
    }

    template <class L>
    auto fixed_width_binary_reference<L>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <class L>
    auto fixed_width_binary_reference<L>::end() const -> const_iterator
    {
        return cend();
    }

    template <class L>
    auto fixed_width_binary_reference<L>::cbegin() const -> const_iterator
    {
        return const_iterator(p_layout->data(offset(m_index)));
    }

    template <class L>
    auto fixed_width_binary_reference<L>::cend() const -> const_iterator
    {
        return const_iterator(p_layout->data(offset(m_index + 1)));
    }

    template <class L>
    template <std::ranges::input_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    bool fixed_width_binary_reference<L>::operator==(const T& rhs) const
    {
        return std::equal(cbegin(), cend(), std::cbegin(rhs), std::cend(rhs));
    }

    template <class L>
    template <std::ranges::input_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    auto fixed_width_binary_reference<L>::operator<=>(const T& rhs) const
    {
        return lexicographical_compare_three_way(*this, rhs);
    }

    template <class L>
    auto fixed_width_binary_reference<L>::offset(size_type index) const -> size_type
    {
        return p_layout->m_element_size * index;
    }
}

#if defined(__cpp_lib_format)

template <typename Layout>
struct std::formatter<sparrow::fixed_width_binary_reference<Layout>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::fixed_width_binary_reference<Layout>& ref, std::format_context& ctx) const
    {
        std::for_each(
            ref.cbegin(),
            sparrow::next(ref.cbegin(), ref.size() - 1),
            [&ctx](const auto& value)
            {
                std::format_to(ctx.out(), "{}, ", value);
            }
        );

        return std::format_to(ctx.out(), "{}>", *std::prev(ref.cend()));
    }
};

template <typename Layout>
inline std::ostream& operator<<(std::ostream& os, const sparrow::fixed_width_binary_reference<Layout>& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
