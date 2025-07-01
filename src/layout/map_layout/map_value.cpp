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

#include "sparrow/layout/map_layout/map_value.hpp"

#include <variant>

#include "sparrow/layout/dispatch.hpp"
#include "sparrow/layout/nested_value_types.hpp"

namespace sparrow
{
    map_value::map_value(
        const array_wrapper* flat_keys,
        const array_wrapper* flat_items,
        size_type index_begin,
        size_type index_end,
        bool keys_sorted
    )
        : p_flat_keys(flat_keys)
        , p_flat_items(flat_items)
        , m_index_begin(index_begin)
        , m_index_end(index_end)
        , m_keys_sorted(keys_sorted)
    {
    }

    bool map_value::empty() const noexcept
    {
        return m_index_begin == m_index_end;
    }

    auto map_value::size() const noexcept -> size_type
    {
        return m_index_end - m_index_begin;
    }

    auto map_value::at(const key_type& key) const -> const_mapped_reference
    {
        return (*this)[key];
    }

    auto map_value::operator[](const key_type& key) const -> const_mapped_reference
    {
        size_type index = find_index(key);
        if (index == m_index_end)
        {
            throw std::out_of_range("key not found in map");
        }
        return array_element(*p_flat_items, index);
    }

    auto map_value::begin() const -> const_iterator
    {
        return cbegin();
    }

    auto map_value::cbegin() const -> const_iterator
    {
        return const_iterator(functor_type(this), 0);
    }

    auto map_value::end() const -> const_iterator
    {
        return cend();
    }

    auto map_value::cend() const -> const_iterator
    {
        return const_iterator(functor_type(this), size());
    }

    auto map_value::rbegin() const -> const_reverser_iterator
    {
        return crbegin();
    }

    auto map_value::crbegin() const -> const_reverser_iterator
    {
        return const_reverser_iterator(cend());
    }

    auto map_value::rend() const -> const_reverser_iterator
    {
        return crend();
    }

    auto map_value::crend() const -> const_reverser_iterator
    {
        return const_reverser_iterator(cbegin());
    }

    auto map_value::value(size_type i) const -> const_reference
    {
        return std::make_pair(
            array_element(*p_flat_keys, i + m_index_begin),
            array_element(*p_flat_items, i + m_index_begin)
        );
    }

    bool operator==(const map_value& lhs, const map_value& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }

    namespace
    {
        template <class K, class A>
        std::size_t
        find_index_impl(const K& key, const A& ar, std::size_t index_begin, std::size_t index_end, const std::false_type&)
        {
            for (std::size_t i = index_begin; i != index_end; ++i)
            {
                const auto& val = ar[i];
                using U = std::decay_t<decltype(val)>;
                if constexpr (mpl::weakly_equality_comparable_with<K, U>)
                {
                    if (val == key)
                    {
                        return i;
                    }
                }
            }
            return index_end;
        }

        template <class K, class A>
        std::size_t
        find_index_impl(const K& key, const A& ar, std::size_t index_begin, std::size_t index_end, const std::true_type&)
        {
            // The initial implementation below has been removed on purpose
            // It increases the size of the library by a factor 3.5 because
            // of all the possible combinations of the variants. Besides, it
            // does not really make sense to use variant values for the keys.
            // We ensure this by throwing in the constructor of map_array.
            // Even if this function is not supposed to be called at runtime,
            // we need to implement it so that the compiler does not complain
            // when building all the branches of find_index.
            //
            // Initial implementation:
            // for (std::size_t i = index_begin; i != index_end; ++i)
            // {
            //    bool res = std::visit([&key]<class T>(const T& val) {
            //        if constexpr (mpl::weakly_equality_comparable_with<T, K>)
            //        {
            //            return val == key;
            //        }
            //        return false;
            //    }, ar[i]);
            //    if (res)
            //    {
            //        return i;
            //    }
            // }

            // Returns index_end, meaning the key is never found.
            return index_end;
        }
    }

    auto map_value::find_index(const key_type& key) const noexcept -> size_type
    {
#if SPARROW_GCC_11_2_WORKAROUND
        using variant_type = std::decay_t<decltype(key)>;
        using base_variant_type = variant_type::base_type;
#endif
        return std::visit(
            [this](const auto& k)
            {
                return visit(
                    [&k, this]<class Ar>(const Ar& ar)
                    {
                        using dispatch_tag = mpl::is_type_instance_of<typename Ar::value_type, nullable_variant>;
                        return find_index_impl(k, ar, m_index_begin, m_index_end, dispatch_tag());
                    },
                    *p_flat_keys
                );
#if SPARROW_GCC_11_2_WORKAROUND
            },
            static_cast<const base_variant_type&>(key)
        );
#else
            },
            key
        );
#endif
    }
}


#if defined(__cpp_lib_format)

auto std::formatter<sparrow::map_value>::format(const sparrow::map_value& map_value, std::format_context& ctx) const
    -> decltype(ctx.out())
{
    std::format_to(ctx.out(), "<");
    if (!map_value.empty())
    {
        for (auto iter = map_value.cbegin(); iter != map_value.cend(); ++iter)
        {
            std::format_to(ctx.out(), "{}: {}, ", iter->first, iter->second);
        }
    }
    return std::format_to(ctx.out(), ">");
}

namespace sparrow
{
    std::ostream& operator<<(std::ostream& os, const sparrow::map_value& value)
    {
        os << std::format("{}", value);
        return os;
    }
}

#endif
