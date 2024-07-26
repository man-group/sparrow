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

#include <concepts>
#include <sstream>
#include <variant>

#include "sparrow/array/typed_array.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /*
     * List of typed_array / external_type_array types
     */
    template <class T>
    using make_typed_array_t = typed_array<T>;
    using all_array_types_t = mpl::transform<make_typed_array_t, all_base_types_t>;

    template <class T>
    using make_external_typed_array_t = external_typed_array<T>;
    using all_external_array_types_t = mpl::transform<make_external_typed_array_t, all_base_types_t>;

    /*
     * Selection of list of typed_array types according to the DataStorage
     */
    template <class DataStorage>
    struct select_all_array_types;

    template <>
    struct select_all_array_types<array_data>
    {
        using type = all_array_types_t;
    };

    template <>
    struct select_all_array_types<external_array_data>
    {
        using type = all_external_array_types_t;
    };

    template <class DataStorage>
    using select_all_array_types_t = typename select_all_array_types<DataStorage>::type;

    /*
     * Common traits class for array and external_array
     */
    template <class DataStorage>
    struct array_traits
    {
        using typed_array_list = select_all_array_types_t<DataStorage>;
        using array_variant = mpl::rename<typed_array_list, std::variant>;

        using value_type = mpl::transform<array_value_type_t, array_variant>;
        using reference = mpl::transform<array_reference_t, array_variant>;
        using const_reference = mpl::transform<array_const_reference_t, array_variant>;

        using inner_iterator = mpl::transform<array_iterator_t, array_variant>;
        using inner_const_iterator = mpl::transform<array_const_iterator_t, array_variant>;
    };

    /*
     * Common iterator class for array and external_array
     */
    namespace impl
    {
        template <class DataStorage, bool is_const>
        using select_reference_t = std::conditional_t<
            is_const,
            typename array_traits<DataStorage>::const_reference,
            typename array_traits<DataStorage>::reference
        >;

        template <class DataStorage, bool is_const>
        using select_inner_iterator_t = std::conditional_t<
            is_const,
            typename array_traits<DataStorage>::inner_const_iterator,
            typename array_traits<DataStorage>::inner_iterator
        >;
    }

    template <class DataStorage, bool is_const>
    class array_iterator_impl
        : public iterator_base<
              array_iterator_impl<DataStorage, is_const>,
              typename array_traits<DataStorage>::value_type,
              std::random_access_iterator_tag,
              impl::select_reference_t<DataStorage, is_const>>
    {
    public:

        using self_type = array_iterator_impl<DataStorage, is_const>;
        using base_type = iterator_base<
            array_iterator_impl<DataStorage, is_const>,
            typename array_traits<DataStorage>::value_type,
            std::random_access_iterator_tag,
            impl::select_reference_t<DataStorage, is_const>>;
        using reference = typename base_type::reference;
        using difference_type = typename base_type::difference_type;

        array_iterator_impl() = default;

        template <class It>
            requires(!std::same_as<std::decay_t<It>, array_iterator_impl<DataStorage, is_const>>)
        array_iterator_impl(It&& iter)
            : m_iter(std::forward<It>(iter))
        {
        }

    private:

        std::string get_type_name() const;
        std::string build_mismatch_message(std::string_view method, const self_type& rhs) const;

        reference dereference() const;
        void increment();
        void decrement();
        void advance(difference_type n);
        difference_type distance_to(const self_type& rhs) const;
        bool equal(const self_type& rhs) const;
        bool less_than(const self_type& rhs) const;

        using inner_iterator = impl::select_inner_iterator_t<DataStorage, is_const>;
        inner_iterator m_iter;

        friend class iterator_access;
    };

    template <bool is_const>
    using array_iterator = array_iterator_impl<array_data, is_const>;

    template <bool is_const>
    using external_array_iterator = array_iterator_impl<external_array_data, is_const>;

    /**
     * common function for building internal storage
     * of array and external_array
     */

    template <class DataStorage>
    typename array_traits<DataStorage>::array_variant
    build_array_variant(DataStorage&& data);
    
    /*********************************
     * array_iterator implementation *
     *********************************/

    template <class DS, bool IC>
    std::string array_iterator_impl<DS, IC>::get_type_name() const
    {
        return std::visit(
            [](auto&& arg)
            {
                return typeid(std::decay_t<decltype(arg)>).name();
            },
            m_iter
        );
    }

    template <class DS, bool IC>
    std::string array_iterator_impl<DS, IC>::build_mismatch_message(std::string_view method, const self_type& rhs) const
    {
        std::ostringstream oss117;
        oss117 << method << ": iterators must have the same type, got " << get_type_name() << " and "
               << rhs.get_type_name();
        return oss117.str();
    }

    template <class DS, bool IC>
    auto array_iterator_impl<DS, IC>::dereference() const -> reference
    {
        return std::visit(
            [](auto&& arg)
            {
                return reference(*arg);
            },
            m_iter
        );
    }

    template <class DS, bool IC>
    void array_iterator_impl<DS, IC>::increment()
    {
        std::visit(
            [](auto&& arg)
            {
                ++arg;
            },
            m_iter
        );
    }

    template <class DS, bool IC>
    void array_iterator_impl<DS, IC>::decrement()
    {
        std::visit(
            [](auto&& arg)
            {
                --arg;
            },
            m_iter
        );
    }

    template <class DS, bool IC>
    void array_iterator_impl<DS, IC>::advance(difference_type n)
    {
        std::visit(
            [n](auto&& arg)
            {
                arg += n;
            },
            m_iter
        );
    }

    template <class DS, bool IC>
    auto array_iterator_impl<DS, IC>::distance_to(const self_type& rhs) const -> difference_type
    {
        if (m_iter.index() != rhs.m_iter.index())
        {
            throw std::invalid_argument(build_mismatch_message("array_iterator::distance_to", rhs));
        }

        return std::visit(
            [&rhs](auto&& arg)
            {
                return std::get<std::decay_t<decltype(arg)>>(rhs.m_iter) - arg;
            },
            m_iter
        );
    }

    template <class DS, bool IC>
    bool array_iterator_impl<DS, IC>::equal(const self_type& rhs) const
    {
        if (m_iter.index() != rhs.m_iter.index())
        {
            return false;
        }

        return std::visit(
            [&rhs](auto&& arg)
            {
                return arg == std::get<std::decay_t<decltype(arg)>>(rhs.m_iter);
            },
            m_iter
        );
    }

    template <class DS, bool IC>
    bool array_iterator_impl<DS, IC>::less_than(const self_type& rhs) const
    {
        if (m_iter.index() != rhs.m_iter.index())
        {
            throw std::invalid_argument(build_mismatch_message("array_iterator::less_than", rhs));
        }

        return std::visit(
            [&rhs](auto&& arg)
            {
                return arg <= std::get<std::decay_t<decltype(arg)>>(rhs.m_iter);
            },
            m_iter
        );
    }

    /**************************************
     * build_array_variant implementation *
     **************************************/

    namespace impl
    {
        template <class DataStorage>
        struct select_typed_array;

        template <>
        struct select_typed_array<array_data>
        {
            template <class T>
            using type = typed_array<T>;
        };

        template <>
        struct select_typed_array<external_array_data>
        {
            template <class T>
            using type = external_typed_array<T>;
        };

        template <class T, class DataStorage>
        using select_typed_array_t = typename select_typed_array<DataStorage>::template type<T>;
    }
    template <class DataStorage>
    inline typename array_traits<DataStorage>::array_variant
    build_array_variant(DataStorage&& data)
    {
        data_descriptor dd = type_descriptor(data);
        switch (dd.id())
        {
            case data_type::NA:
                return impl::select_typed_array_t<null_type, DataStorage>(std::move(data));
            case data_type::BOOL:
                return impl::select_typed_array_t<bool, DataStorage>(std::move(data));
            case data_type::UINT8:
                return impl::select_typed_array_t<std::uint8_t, DataStorage>(std::move(data));
            case data_type::INT8:
                return impl::select_typed_array_t<std::int8_t, DataStorage>(std::move(data));
            case data_type::UINT16:
                return impl::select_typed_array_t<std::uint16_t, DataStorage>(std::move(data));
            case data_type::INT16:
                return impl::select_typed_array_t<std::int16_t, DataStorage>(std::move(data));
            case data_type::UINT32:
                return impl::select_typed_array_t<std::uint32_t, DataStorage>(std::move(data));
            case data_type::INT32:
                return impl::select_typed_array_t<std::int32_t, DataStorage>(std::move(data));
            case data_type::UINT64:
                return impl::select_typed_array_t<std::uint64_t, DataStorage>(std::move(data));
            case data_type::INT64:
                return impl::select_typed_array_t<std::int64_t, DataStorage>(std::move(data));
            case data_type::HALF_FLOAT:
                return impl::select_typed_array_t<float16_t, DataStorage>(std::move(data));
            case data_type::FLOAT:
                return impl::select_typed_array_t<float32_t, DataStorage>(std::move(data));
            case data_type::DOUBLE:
                return impl::select_typed_array_t<float64_t, DataStorage>(std::move(data));
            case data_type::STRING:
            case data_type::FIXED_SIZE_BINARY:
                return impl::select_typed_array_t<std::string, DataStorage>(std::move(data));
            case data_type::TIMESTAMP:
                return impl::select_typed_array_t<sparrow::timestamp, DataStorage>(std::move(data));
            default:
                // TODO: implement other data types, remove the default use case
                // and throw from outside of the switch
                throw std::invalid_argument("not supported yet");
        }
    }
}
