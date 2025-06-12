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

#if defined(__cpp_lib_format)
#    include <format>
#endif

#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{
    class struct_value;

    class SPARROW_API struct_value_iterator : public iterator_base<
                                                  struct_value_iterator,
                                                  array_traits::const_reference,
                                                  std::random_access_iterator_tag,
                                                  array_traits::const_reference>
    {
    public:

        using self_type = struct_value_iterator;
        using base_type = iterator_base<struct_value_iterator, struct_value, std::random_access_iterator_tag>;
        using size_type = size_t;

        struct_value_iterator() noexcept = default;
        struct_value_iterator(const struct_value* struct_value_ptr, size_type index);

    private:

        [[nodiscard]] reference dereference() const;

        void increment();
        void decrement();
        void advance(difference_type n);
        [[nodiscard]] difference_type distance_to(const self_type& rhs) const;
        [[nodiscard]] bool equal(const self_type& rhs) const;
        [[nodiscard]] bool less_than(const self_type& rhs) const;

        const struct_value* m_struct_value_ptr = nullptr;
        difference_type m_index;

        friend class iterator_access;
    };

    class SPARROW_API struct_value
    {
    public:

        using value_type = array_traits::value_type;
        using const_reference = array_traits::const_reference;
        using size_type = std::size_t;
        using child_ptr = cloning_ptr<array_wrapper>;

        struct_value() = default;
        struct_value(const std::vector<child_ptr>& children, size_type index);

        [[nodiscard]] size_type size() const;
        [[nodiscard]] bool empty() const;

        [[nodiscard]] const_reference operator[](size_type i) const;

        [[nodiscard]] const_reference front() const;
        [[nodiscard]] const_reference back() const;

        [[nodiscard]] struct_value_iterator begin();
        [[nodiscard]] struct_value_iterator begin() const;
        [[nodiscard]] struct_value_iterator cbegin() const;

        [[nodiscard]] struct_value_iterator end();
        [[nodiscard]] struct_value_iterator end() const;
        [[nodiscard]] struct_value_iterator cend() const;

    private:

        const std::vector<child_ptr>* p_children = nullptr;
        size_type m_index = 0u;
    };

    SPARROW_API
    bool operator==(const struct_value& lhs, const struct_value& rhs);
}

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::struct_value>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    SPARROW_API auto format(const sparrow::struct_value& ar, std::format_context& ctx) const
        -> decltype(ctx.out());
};

namespace sparrow
{
    SPARROW_API std::ostream& operator<<(std::ostream& os, const sparrow::struct_value& value);
}

#endif
