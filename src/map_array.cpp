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

#include "sparrow/map_array.hpp"

#include "sparrow/array.hpp"
#include "sparrow/debug/copy_tracker.hpp"

namespace sparrow
{
    namespace copy_tracker
    {
        template <>
        SPARROW_API std::string key<map_array>()
        {
            return "map_array";
        }
    }

    map_array::map_array(arrow_proxy proxy)
        : base_type(std::move(proxy))
        , p_list_offsets(make_list_offsets())
        , p_entries_array(make_entries_array())
        , m_keys_sorted(get_keys_sorted())
    {
        visit(
            []<class T>(const T&)
            {
                using value_type = typename T::value_type;
                if constexpr (mpl::is_type_instance_of<value_type, nullable_variant>::value)
                {
                    throw std::runtime_error("Array of variants cannot be used as array of keys in a map_array");
                }
            },
            *(raw_keys_array())
        );
    }

    map_array::map_array(const self_type& rhs)
        : base_type(rhs)
        , p_list_offsets(make_list_offsets())
        , p_entries_array(make_entries_array())
        , m_keys_sorted(rhs.m_keys_sorted)
    {
        copy_tracker::increase(copy_tracker::key<map_array>());
    }

    map_array& map_array::operator=(const self_type& rhs)
    {
        copy_tracker::increase(copy_tracker::key<map_array>());
        if (this != &rhs)
        {
            base_type::operator=(rhs);
            p_list_offsets = make_list_offsets();
            p_entries_array = make_entries_array();
            m_keys_sorted = rhs.m_keys_sorted;
        }
        return *this;
    }

    const array_wrapper* map_array::raw_keys_array() const
    {
        return unwrap_array<struct_array>(*p_entries_array).raw_child(std::size_t(0));
        ;
    }

    array_wrapper* map_array::raw_keys_array()
    {
        return unwrap_array<struct_array>(*p_entries_array).raw_child(std::size_t(0));
    }

    const array_wrapper* map_array::raw_items_array() const
    {
        return unwrap_array<struct_array>(*p_entries_array).raw_child(std::size_t(1));
    }

    array_wrapper* map_array::raw_items_array()
    {
        return unwrap_array<struct_array>(*p_entries_array).raw_child(std::size_t(1));
    }

    auto map_array::value_begin() -> value_iterator
    {
        return value_iterator(value_iterator::functor_type(this), 0);
    }

    auto map_array::value_end() -> value_iterator
    {
        return value_iterator(value_iterator::functor_type(this), this->size());
    }

    auto map_array::value_cbegin() const -> const_value_iterator
    {
        return const_value_iterator(const_value_iterator::functor_type(this), 0);
    }

    auto map_array::value_cend() const -> const_value_iterator
    {
        return const_value_iterator(const_value_iterator::functor_type(this), this->size());
    }

    auto map_array::value(size_type i) -> inner_reference
    {
        return static_cast<const map_array*>(this)->value(i);
    }

    auto map_array::value(size_type i) const -> inner_const_reference
    {
        return map_value(raw_keys_array(), raw_items_array(), p_list_offsets[i], p_list_offsets[i + 1], m_keys_sorted);
    }

    auto map_array::make_list_offsets() const -> offset_type*
    {
        return reinterpret_cast<offset_type*>(
            this->get_arrow_proxy().buffers()[OFFSET_BUFFER_INDEX].data() + this->get_arrow_proxy().offset()
        );
    }

    cloning_ptr<array_wrapper> map_array::make_entries_array() const
    {
        return array_factory(this->get_arrow_proxy().children()[0].view());
    }

    bool map_array::get_keys_sorted() const
    {
        return this->get_arrow_proxy().flags().contains(ArrowFlag::MAP_KEYS_SORTED);
    }

    bool map_array::check_keys_sorted(const array& flat_keys, const offset_buffer_type& offsets)
    {
        bool sorted = true;
        for (std::size_t i = 0; i + 1 < offsets.size(); ++i)
        {
            std::size_t index_begin = offsets[i];
            std::size_t index_end = offsets[i + 1];
            sorted = flat_keys.visit(
                [index_begin, index_end]<class T>(const T& ar) -> bool
                {
                    bool isorted = false;
                    if constexpr (std::three_way_comparable<typename T::const_reference>)
                    {
                        isorted = true;
                        for (std::size_t j = index_begin; j + 1 < index_end; ++j)
                        {
                            isorted = (ar[j] < ar[j + 1]);
                            if (!isorted)
                            {
                                break;
                            }
                        }
                    }
                    return isorted;
                }
            );
            if (!sorted)
            {
                break;
            }
        }
        return sorted;
    }
}
