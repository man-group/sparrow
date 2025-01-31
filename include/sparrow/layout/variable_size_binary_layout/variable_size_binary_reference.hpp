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

#include <algorithm>
#include <concepts>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>

#if defined(__cpp_lib_format)
#    include <format>
#    include <ostream>
#endif

#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * Implementation of reference to inner type used for layout L
     *
     * @tparam L the layout type
     */
    template <class L>
    class variable_size_binary_reference
    {
    public:

        using self_type = variable_size_binary_reference<L>;
        using value_type = typename L::inner_value_type;
        using reference = typename L::inner_reference;
        using const_reference = typename L::inner_const_reference;
        using size_type = typename L::size_type;
        using difference_type = std::ptrdiff_t;
        using iterator = typename L::data_iterator;
        using const_iterator = typename L::const_data_iterator;
        using offset_type = typename L::offset_type;

        variable_size_binary_reference(L* layout, size_type index);
        variable_size_binary_reference(const variable_size_binary_reference&) = default;
        variable_size_binary_reference(variable_size_binary_reference&&) = default;

        template <std::ranges::sized_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        self_type& operator=(T&& rhs);

        // This is to avoid const char* from begin caught by the previous
        // operator= overload. It would convert const char* to const char[N],
        // including the null-terminating char.
        template <class U = typename L::inner_value_type>
            requires std::assignable_from<U&, const char*>
        self_type& operator=(const char* rhs);

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

        template <class U = typename L::inner_value_type>
            requires std::assignable_from<U&, const char*>
        bool operator==(const char* rhs) const;

        template <std::ranges::input_range T>
            requires mpl::convertible_ranges<T, typename L::inner_value_type>
        auto operator<=>(const T& rhs) const;

        template <class U = typename L::inner_value_type>
            requires std::assignable_from<U&, const char*>
        auto operator<=>(const char* rhs) const;

    private:

        offset_type offset(size_type index) const;
        size_type uoffset(size_type index) const;

        L* p_layout = nullptr;
        size_type m_index = size_type(0);
    };
}

namespace std
{
    template <typename Layout, template <typename> typename TQual, template <typename> typename UQual>
    struct basic_common_reference<sparrow::variable_size_binary_reference<Layout>, std::string, TQual, UQual>
    {
        using type = std::string;
    };

    template <typename Layout, template <typename> typename TQual, template <class> class UQual>
    struct basic_common_reference<std::string, sparrow::variable_size_binary_reference<Layout>, TQual, UQual>
    {
        using type = std::string;
    };

    template <typename Layout, template <typename> typename TQual, template <typename> typename UQual>
    struct basic_common_reference<sparrow::variable_size_binary_reference<Layout>, std::vector<std::byte>, TQual, UQual>
    {
        using type = std::vector<std::byte>;
    };

    template <typename Layout, template <typename> typename TQual, template <class> class UQual>
    struct basic_common_reference<std::vector<std::byte>, sparrow::variable_size_binary_reference<Layout>, TQual, UQual>
    {
        using type = std::vector<std::byte>;
    };
}

namespace sparrow
{
    /*************************************************
     * variable_size_binary_reference implementation *
     *************************************************/

    template <class L>
    variable_size_binary_reference<L>::variable_size_binary_reference(L* layout, size_type index)
        : p_layout(layout)
        , m_index(index)
    {
    }

    template <class L>
    template <std::ranges::sized_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    auto variable_size_binary_reference<L>::operator=(T&& rhs) -> self_type&
    {
        p_layout->assign(std::forward<T>(rhs), m_index);
        p_layout->get_arrow_proxy().update_buffers();
        return *this;
    }

    template <class L>
    template <class U>
        requires std::assignable_from<U&, const char*>
    auto variable_size_binary_reference<L>::operator=(const char* rhs) -> self_type&
    {
        return *this = std::string_view(rhs);
    }

    template <class L>
    auto variable_size_binary_reference<L>::size() const -> size_type
    {
        return static_cast<size_type>(offset(m_index + 1) - offset(m_index));
    }

    template <class L>
    auto variable_size_binary_reference<L>::begin() -> iterator
    {
        return iterator(p_layout->data(uoffset(m_index)));
    }

    template <class L>
    auto variable_size_binary_reference<L>::end() -> iterator
    {
        return iterator(p_layout->data(uoffset(m_index + 1)));
    }

    template <class L>
    auto variable_size_binary_reference<L>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <class L>
    auto variable_size_binary_reference<L>::end() const -> const_iterator
    {
        return cend();
    }

    template <class L>
    auto variable_size_binary_reference<L>::cbegin() const -> const_iterator
    {
        return const_iterator(p_layout->data(uoffset(m_index)));
    }

    template <class L>
    auto variable_size_binary_reference<L>::cend() const -> const_iterator
    {
        return const_iterator(p_layout->data(uoffset(m_index + 1)));
    }

    template <class L>
    template <std::ranges::input_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    bool variable_size_binary_reference<L>::operator==(const T& rhs) const
    {
        return std::equal(cbegin(), cend(), std::cbegin(rhs), std::cend(rhs));
    }

    template <class L>
    template <class U>
        requires std::assignable_from<U&, const char*>
    bool variable_size_binary_reference<L>::operator==(const char* rhs) const
    {
        return operator==(std::string_view(rhs));
    }

    template <class L>
    template <std::ranges::input_range T>
        requires mpl::convertible_ranges<T, typename L::inner_value_type>
    auto variable_size_binary_reference<L>::operator<=>(const T& rhs) const
    {
        return lexicographical_compare_three_way(*this, rhs);
    }

    template <class L>
    template <class U>
        requires std::assignable_from<U&, const char*>
    auto variable_size_binary_reference<L>::operator<=>(const char* rhs) const
    {
        return operator<=>(std::string_view(rhs));
    }

    template <class L>
    auto variable_size_binary_reference<L>::offset(size_type index) const -> offset_type
    {
        return *(p_layout->offset(index));
    }

    template <class L>
    auto variable_size_binary_reference<L>::uoffset(size_type index) const -> size_type
    {
        return static_cast<size_type>(offset(index));
    }
}

#if defined(__cpp_lib_format)
template <typename Layout>
struct std::formatter<sparrow::variable_size_binary_reference<Layout>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::variable_size_binary_reference<Layout>& ref, std::format_context& ctx) const
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
inline std::ostream& operator<<(std::ostream& os, const sparrow::variable_size_binary_reference<Layout>& value)
{
    os << std::format("{}", value);
    return os;
}

#endif
