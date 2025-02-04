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

#include "sparrow/array_api.hpp"
#include "sparrow/array_factory.hpp"
#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/array_base.hpp"
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/layout/primitive_array.hpp"
#include "sparrow/types/data_traits.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/functor_index_iterator.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{
    template <class Layout, bool is_const>
    class layout_element_functor
    {
    public:

        using layout_type = Layout;
        using storage_type = std::conditional_t<is_const, const layout_type*, layout_type>;
        using return_type = std::
            conditional_t<is_const, typename layout_type::const_reference, typename layout_type::reference>;

        constexpr layout_element_functor() = default;

        constexpr explicit layout_element_functor(storage_type layout_)
            : p_layout(layout_)
        {
        }

        return_type operator()(std::size_t i) const
        {
            return p_layout->operator[](i);
        }

    private:

        storage_type p_layout;
    };

    template <std::integral IT>
    class dictionary_encoded_array;

    namespace detail
    {
        template <class T>
        struct get_data_type_from_array;

        template <std::integral IT>
        struct get_data_type_from_array<sparrow::dictionary_encoded_array<IT>>
        {
            static constexpr sparrow::data_type get()
            {
                return arrow_traits<typename primitive_array<IT>::inner_value_type>::type_id;
            }
        };

        template <std::integral IT>
        struct is_dictionary_encoded_array<sparrow::dictionary_encoded_array<IT>>
        {
            static constexpr bool get()
            {
                return true;
            }
        };
    }

    /**
     * Checks whether T is a dictionary_encoded_array type.
     */
    template <class T>
    constexpr bool is_dictionary_encoded_array_v = detail::is_dictionary_encoded_array<T>::get();

    template <std::integral IT>
    class dictionary_encoded_array
    {
    public:

        using self_type = dictionary_encoded_array<IT>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        using inner_value_type = array_traits::inner_value_type;

        using value_type = array_traits::value_type;
        using reference = array_traits::const_reference;
        using const_reference = array_traits::const_reference;

        using functor_type = layout_element_functor<self_type, true>;
        using const_functor_type = layout_element_functor<self_type, true>;

        using iterator = functor_index_iterator<functor_type>;
        using const_iterator = functor_index_iterator<const_functor_type>;

        using keys_buffer_type = u8_buffer<IT>;

        explicit dictionary_encoded_array(arrow_proxy);

        dictionary_encoded_array(const self_type&);
        self_type& operator=(const self_type&);

        dictionary_encoded_array(self_type&&);
        self_type& operator=(self_type&&);

        std::optional<std::string_view> name() const;
        std::optional<std::string_view> metadata() const;

        size_type size() const;
        bool empty() const;

        const_reference operator[](size_type i) const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;

        const_iterator cbegin() const;
        const_iterator cend() const;

        [[nodiscard]] const_reference front() const;
        [[nodiscard]] const_reference back() const;

        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<dictionary_encoded_array<IT>, Args...>)
        explicit dictionary_encoded_array(Args&&... args)
            : dictionary_encoded_array(create_proxy(std::forward<Args>(args)...))
        {
        }

        /**
         * Slices the array to keep only the elements between the given \p start and \p end.
         * A copy of the \ref array is modified. The data is not modified, only the ArrowArray.offset and
         * ArrowArray.length are updated. If \p end is greater than the size of the buffers, the following
         * elements will be invalid.
         *
         * @param start The index of the first element to keep. Must be less than \p end.
         * @param end The index of the first element to discard. Must be less than the size of the buffers.
         */
        self_type slice(size_type start, size_type end) const;

        /**
         * Slices the array to keep only the elements between the given \p start and \p end.
         * A view of the \ref array is returned. The data is not modified, only the ArrowArray.offset and
         * ArrowArray.length are updated. If \p end is greater than the size of the buffers, the following
         * elements will be invalid.
         *
         * @param start The index of the first element to keep. Must be less than \p end.
         * @param end The index of the first element to discard. Must be less than the size of the buffers.
         */
        self_type slice_view(size_type start, size_type end) const;

    private:

        template <validity_bitmap_input R = validity_bitmap>
        static auto create_proxy(
            keys_buffer_type&& keys,
            array&& values,
            R&& bitmaps = validity_bitmap{},
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::string_view> metadata = std::nullopt
        ) -> arrow_proxy;

        using keys_layout = primitive_array<IT>;
        using values_layout = cloning_ptr<array_wrapper>;

        [[nodiscard]] const inner_value_type& dummy_inner_value() const;
        // inner_const_reference dummy_inner_const_reference() const;
        [[nodiscard]] const_reference dummy_const_reference() const;

        static keys_layout create_keys_layout(arrow_proxy& proxy);
        static values_layout create_values_layout(arrow_proxy& proxy);

        [[nodiscard]] arrow_proxy& get_arrow_proxy();
        [[nodiscard]] const arrow_proxy& get_arrow_proxy() const;

        arrow_proxy m_proxy;
        keys_layout m_keys_layout;
        values_layout p_values_layout;

        friend class detail::array_access;
    };

    template <class IT>
    bool operator==(const dictionary_encoded_array<IT>& lhs, const dictionary_encoded_array<IT>& rhs);

    /*******************************************
     * dictionary_encoded_array implementation *
     *******************************************/

    template <std::integral IT>
    dictionary_encoded_array<IT>::dictionary_encoded_array(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
        , m_keys_layout(create_keys_layout(m_proxy))
        , p_values_layout(create_values_layout(m_proxy))
    {
        SPARROW_ASSERT_TRUE(data_type_is_integer(m_proxy.data_type()));
    }

    template <std::integral IT>
    dictionary_encoded_array<IT>::dictionary_encoded_array(const self_type& rhs)
        : m_proxy(rhs.m_proxy)
        , m_keys_layout(create_keys_layout(m_proxy))
        , p_values_layout(create_values_layout(m_proxy))
    {
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            m_proxy = rhs.m_proxy;
            m_keys_layout = create_keys_layout(m_proxy);
            p_values_layout = create_values_layout(m_proxy);
        }
        return *this;
    }

    template <std::integral IT>
    dictionary_encoded_array<IT>::dictionary_encoded_array(self_type&& rhs)
        : m_proxy(std::move(rhs.m_proxy))
        , m_keys_layout(create_keys_layout(m_proxy))
        , p_values_layout(create_values_layout(m_proxy))
    {
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::operator=(self_type&& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            using std::swap;
            swap(m_proxy, rhs.m_proxy);
            m_keys_layout = create_keys_layout(m_proxy);
            p_values_layout = create_values_layout(m_proxy);
        }
        return *this;
    }

    template <std::integral IT>
    template <validity_bitmap_input VBI>
    auto dictionary_encoded_array<IT>::create_proxy(
        keys_buffer_type&& keys,
        array&& values,
        VBI&& validity_input,
        std::optional<std::string_view> name,
        std::optional<std::string_view> metadata
    ) -> arrow_proxy
    {
        const auto size = keys.size();
        validity_bitmap vbitmap = ensure_validity_bitmap(size, std::forward<VBI>(validity_input));

        auto [value_array, value_schema] = extract_arrow_structures(std::move(values));
        const auto null_count = vbitmap.null_count();

        // create arrow schema and array
        ArrowSchema schema = make_arrow_schema(
            sparrow::data_type_format_of<IT>(),
            std::move(name),                          // name
            std::move(metadata),                      // metadata
            std::nullopt,                             // flags
            0,                                        // n_children
            nullptr,                                  // children
            new ArrowSchema(std::move(value_schema))  // dictionary
        );

        std::vector<buffer<uint8_t>> buffers(2);
        buffers[0] = std::move(vbitmap).extract_storage();
        buffers[1] = std::move(keys).extract_storage();

        // create arrow array
        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(size),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(buffers),
            0,                                      // n_children
            nullptr,                                // children
            new ArrowArray(std::move(value_array))  // dictionary
        );
        return arrow_proxy(std::move(arr), std::move(schema));
    }

    template <std::integral IT>
    std::optional<std::string_view> dictionary_encoded_array<IT>::name() const
    {
        return m_proxy.name();
    }

    template <std::integral IT>
    std::optional<std::string_view> dictionary_encoded_array<IT>::metadata() const
    {
        return m_proxy.metadata();
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::size() const -> size_type
    {
        return m_proxy.length();
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::empty() const -> bool
    {
        return size() == 0;
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size());
        const auto index = m_keys_layout[i];

        if (index.has_value())
        {
            SPARROW_ASSERT_TRUE(index.value() >= 0);
            return array_element(*p_values_layout, static_cast<std::size_t>(index.value()));
        }
        else
        {
            return dummy_const_reference();
        }
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::begin() -> iterator
    {
        return iterator(functor_type(this), 0u);
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::end() -> iterator
    {
        return iterator(functor_type(this), size());
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::begin() const -> const_iterator
    {
        return cbegin();
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::end() const -> const_iterator
    {
        return cend();
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::cbegin() const -> const_iterator
    {
        return const_iterator(const_functor_type(this), 0u);
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::cend() const -> const_iterator
    {
        return const_iterator(const_functor_type(this), size());
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::front() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return operator[](0);
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::back() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return operator[](size() - 1);
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::dummy_inner_value() const -> const inner_value_type&
    {
        static const inner_value_type instance = array_default_element_value(*p_values_layout);
        return instance;
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::slice(size_type start, size_type end) const -> self_type
    {
        SPARROW_ASSERT_TRUE(start <= end);
        return self_type{get_arrow_proxy().slice(start, end)};
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::slice_view(size_type start, size_type end) const -> self_type
    {
        SPARROW_ASSERT_TRUE(start <= end);
        return self_type{get_arrow_proxy().slice_view(start, end)};
    }

    /*template <std::integral IT>
    auto dictionary_encoded_array<IT>::dummy_inner_const_reference() const -> inner_const_reference
    {
        static const inner_const_reference instance =
            std::visit([](const auto& val) -> inner_const_reference { return val; }, dummy_inner_value());
        return instance;
    }*/

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::dummy_const_reference() const -> const_reference
    {
        static const const_reference instance = std::visit(
            [](const auto& val) -> const_reference
            {
                using inner_ref = typename arrow_traits<std::decay_t<decltype(val)>>::const_reference;
                return const_reference{nullable<inner_ref>(inner_ref(val), false)};
            },
            dummy_inner_value()
        );
        return instance;
    }

    template <std::integral IT>
    typename dictionary_encoded_array<IT>::values_layout
    dictionary_encoded_array<IT>::create_values_layout(arrow_proxy& proxy)
    {
        const auto& dictionary = proxy.dictionary();
        SPARROW_ASSERT_TRUE(dictionary);
        arrow_proxy ar_dictionary{&(dictionary->array()), &(dictionary->schema())};
        return array_factory(std::move(ar_dictionary));
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::create_keys_layout(arrow_proxy& proxy) -> keys_layout
    {
        return keys_layout{arrow_proxy{&proxy.array(), &proxy.schema()}};
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::get_arrow_proxy() -> arrow_proxy&
    {
        return m_proxy;
    }

    template <std::integral IT>
    auto dictionary_encoded_array<IT>::get_arrow_proxy() const -> const arrow_proxy&
    {
        return m_proxy;
    }

    template <class IT>
    bool operator==(const dictionary_encoded_array<IT>& lhs, const dictionary_encoded_array<IT>& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }
}

#if defined(__cpp_lib_format)
template <std::integral IT>
struct std::formatter<sparrow::dictionary_encoded_array<IT>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::dictionary_encoded_array<IT>& ar, std::format_context& ctx) const
    {
        std::format_to(ctx.out(), "Dictionary [size={}] <", ar.size());
        std::for_each(
            ar.cbegin(),
            std::prev(ar.cend()),
            [&ctx](const auto& value)
            {
                std::format_to(ctx.out(), "{}, ", value);
            }
        );
        std::format_to(ctx.out(), "{}>", ar.back());
        return ctx.out();
    }
};

template <std::integral IT>
std::ostream& operator<<(std::ostream& os, const sparrow::dictionary_encoded_array<IT>& value)
{
    os << std::format("{}", value);
    return os;
}
#endif
