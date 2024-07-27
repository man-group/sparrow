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
#include <compare>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <type_traits>
#include <unordered_set>

#include "sparrow/array/array_data.hpp"
#include "sparrow/array/array_data_factory.hpp"
#include "sparrow/array/data_traits.hpp"
#include "sparrow/array/data_type.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/utils/algorithm.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * A class template representing a typed array.
     *
     * The `typed_array_impl` class template provides an container interface over `array_data` for elements of a
     * specific type `T`. The access to the elements are executed according to the layout `L` of the array.
     *
     * @tparam T The type of elements stored in the array.
     * @tparam L The layout type of the array. Defaults to the default layout defined by the `arrow_traits` of
     * `T`.
     */
    template <is_arrow_base_type T, arrow_layout L>
    class typed_array_impl
    {
    public:

        using layout_type = L;
        using data_storage_type = typename layout_type::data_storage_type;

        using value_type = typename layout_type::value_type;
        using reference = typename layout_type::reference;
        using const_reference = typename layout_type::const_reference;

        using iterator = typename layout_type::iterator;
        using const_iterator = typename layout_type::const_iterator;

        using size_type = typename layout_type::size_type;
        using const_bitmap_range = typename layout_type::const_bitmap_range;
        using const_value_range = typename layout_type::const_value_range;

        template <class D = typename L::data_storage_type>
        requires (not std::same_as<D, external_array_data>)
        typed_array_impl();

        explicit typed_array_impl(data_storage_type data);

        typed_array_impl(const typed_array_impl& rhs);
        typed_array_impl(typed_array_impl&& rhs);

        ///@{
        /** Construct a typed array with a fixed layout with the same value repeated `n` times.
         * 
         * @param n The number of elements in the array.
         * @param value The value to repeat.
         */
        ///@}
        template<class U>
        requires std::convertible_to<U, T>
         && mpl::is_type_instance_of_v<L, fixed_size_layout>
        typed_array_impl(size_type n, const U& value);

        // TODO: the commented constructors lead to compilation error when the
        // data_storage_type is external_array_data, probably because of a missing
        // constraint (notice there were added BEFORE the external_array development).
        // However, this doe snot seem trivial to fix (adding a constraint on an
        // additional template parameter that default to data_storage_type is not enough).
        // Maybe a split of this class is required, with a base class capturing the
        // implementation, and inheriting class providing the constructors.
        ///@{
        /**
         * Constructs a typed array with a fixed-size layout from a range of values.
         *
         * The range must be a range of values convertible to `T` and the elements must not be nullable.
         *
         * @tparam R The type of the range.
         * @param range The range of values to construct the array from.
         */
        /*template <class R, class D = typename L::data_storage_type>
        requires std::ranges::range<R> 
            && std::convertible_to<std::ranges::range_value_t<R>, T> 
            && (!is_nullable_v<std::ranges::range_value_t<R>>)
            && mpl::is_type_instance_of_v<L, fixed_size_layout>
        typed_array_impl(R&& range, D = {});*/
        ///@}

        ///@{
        /**
         * Constructs a typed array with a fixed-size layout from a range of nullable values.
         *
         * The range must be a range of values convertible to `T` and the elements must be nullable.
         *
         * @tparam R The type of the range.
         * @param range The range of values to construct the array from.
         */
        /*template <class R>
        requires std::ranges::range<R> 
            && is_nullable_of_convertible_to<std::ranges::range_value_t<R>, T>
            && mpl::is_type_instance_of_v<L, fixed_size_layout>
        typed_array_impl(R&& range);*/
        ///@}

        ///@{
        /**
         * Constructs a typed array with a variable-size layout from a range of values.
         * 
         * The range must be a range of values convertible to `T` and the elements must not be nullable.
         * 
         * @tparam R The type of the range.
         * @param range The range of values to construct the array from.
         */
        /*template <class R>
        requires std::ranges::range<R> 
            && std::convertible_to<std::ranges::range_value_t<R>, T> 
            && (!is_nullable_v<std::ranges::range_value_t<R>>)
            && mpl::is_type_instance_of_v<L, variable_size_binary_layout>
        typed_array_impl(R&& range);*/
        ///@}

        ///@{
        /**
         * Constructs a typed array with a variable-size layout from a range of nullable values.
         * 
         * The range must be a range of values convertible to `T` and the elements must be nullable.
         * 
         * @tparam R The type of the range.
         * @param range The range of values to construct the array from.
         */
        /*template <class R>
        requires std::ranges::range<R> 
            && is_nullable_of_convertible_to<std::ranges::range_value_t<R>, T>
            && mpl::is_type_instance_of_v<L, variable_size_binary_layout>
        typed_array_impl(R&& range);*/
        ///@}

        typed_array_impl& operator=(const typed_array_impl& rhs);
        typed_array_impl& operator=(typed_array_impl&& rhs);

        // Element access

        ///@{
        /*
         * Access specified element with bounds checking.
         *
         * Returns a reference to the element at the specified index \p i, with bounds checking.
         * If \p i is not within the range of the container, an exception of type std::out_of_range is thrown.
         *
         * @param i The index of the element to access.
         * @return A reference to the element at the specified index.
         * @throws std::out_of_range if i is out of range.
         */
        reference at(size_type i);
        const_reference at(size_type i) const;
        ///@}

        ///@{
        /*
         * Access specified element.
         *
         * Returns a reference to the element at the specified index \p i. No bounds checking is performed.
         *
         * @param i The index of the element to access.
         * @pre @p i must be lower than the size of the container.
         * @return A reference to the element at the specified index.
         */
        reference operator[](size_type);
        const_reference operator[](size_type) const;
        ///@}

        ///@{
        /*
         * Access the first element.
         *
         * Returns a reference to the first element.
         * @pre The container must not be empty (\see empty()).
         *
         * @return A reference to the first element.
         */
        reference front();
        const_reference front() const;
        ///@}

        ///@{
        /*
         * Access the last element.
         *
         * Returns a reference to the last element.
         *
         * @pre The container must not be empty (\see empty()).
         *
         * @return A reference to the last element.
         */
        reference back();
        const_reference back() const;
        ///@}

        // Iterators

        ///@{
        /* Returns an iterator to the first element.
         * If the vector is empty, the returned iterator will be equal to end().
         *
         * @return An iterator to the first element.
         */
        iterator begin();
        const_iterator begin() const;
        const_iterator cbegin() const;
        ///@}

        ///@{
        /**
         * This element acts as a placeholder; attempting to access it results in undefined behavior.
         *
         * @return An iterator to the element following the last element of the vector.
         */
        iterator end();
        const_iterator end() const;
        const_iterator cend() const;
        ///@}

        /*
         * @return A range of the bitmap. For each index position in this range, if `true` then there is a
         * value at the same index position in the `values()` range, `false` means the value there is null.
         */
        const_bitmap_range bitmap() const;

        /*
         * @return A range of the values.
         */
        const_value_range values() const;

        // Capacity

        /*
         * @return true if the container is empty, false otherwise.
         */
        bool empty() const;

        /*
         * @return The number of elements in the container.
         */
        size_type size() const;

        // TODO: Add reserve, capacity, shrink_to_fit

        // Modifiers

        // TODO: Implement insert, erase, push_back, pop_back, clear, resize, swap

    private:

        data_storage_type m_data = make_default_array_data<L>();
        layout_type m_layout{m_data};
    };

    template <class T, class Layout>
    std::partial_ordering operator<=>(const typed_array_impl<T, Layout>& ta1, const typed_array_impl<T, Layout>& ta2);

    template <class T, class Layout>
    bool operator==(const typed_array_impl<T, Layout>& ta1, const typed_array_impl<T, Layout>& ta2);
    
    namespace impl
    {
        template <class T, class DataStorage>
        using default_layout = typename arrow_traits<T>::template default_layout<DataStorage>;
    }

    template <class T, class Layout = impl::default_layout<T, array_data>>
    using typed_array = typed_array_impl<T, Layout>;
    
    template <class T, class Layout = impl::default_layout<T, external_array_data>>
    using external_typed_array = typed_array_impl<T, Layout>;


    /*
     * is_typed_array_impl traits
     */
    template <class A>
    struct is_typed_array_impl : std::false_type
    {
    };

    template <class T, class L>
    struct is_typed_array_impl<typed_array_impl<T, L>> : std::true_type
    {
    };

    template <class A>
    constexpr bool is_typed_array_impl_v = is_typed_array_impl<A>::value;

    /*
     * typed_array_impl traits
     */
    template <class A>
    requires is_typed_array_impl_v<A>
    using array_value_type_t = typename A::value_type;

    template <class A>
    requires is_typed_array_impl_v<A>
    using array_reference_t = typename A::reference;

    template <class A>
    requires is_typed_array_impl_v<A>
    using array_const_reference_t = typename A::const_reference;

    template <class A>
    requires is_typed_array_impl_v<A>
    using array_size_type_t = typename A::size_type;

    template <class A>
    requires is_typed_array_impl_v<A>
    using array_iterator_t = typename A::iterator;

    template <class A>
    requires is_typed_array_impl_v<A>
    using array_const_iterator_t = typename A::const_iterator;

    template <class A>
    requires is_typed_array_impl_v<A>
    using array_const_bitmap_range_t = typename A::const_bitmap_range;

    template <class A>
    requires is_typed_array_impl_v<A>
    using array_const_value_range_t = typename A::const_value_range;

    // Constructors

    template <is_arrow_base_type T, arrow_layout L>
    template <class D>
    requires (not std::same_as<D, external_array_data>)
    typed_array_impl<T, L>::typed_array_impl()
        : m_data(make_default_array_data<L>())
        ,m_layout{m_data}
    {
    }
    
    template <is_arrow_base_type T, arrow_layout L>
    typed_array_impl<T, L>::typed_array_impl(data_storage_type data)
        : m_data(std::move(data))
        , m_layout(m_data)
    {
    }

    template <is_arrow_base_type T, arrow_layout L>
    template<class U>
    requires std::convertible_to<U, T>
        && mpl::is_type_instance_of_v<L, fixed_size_layout>
    typed_array_impl<T, L>::typed_array_impl(size_type n, const U& value)
    {
        sparrow::array_data ad;
        ad.type = sparrow::data_descriptor(sparrow::arrow_traits<T>::type_id);
        ad.length = static_cast<typename array_data::length_type>(n);
        ad.offset = static_cast<std::int64_t>(0);
        ad.bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);
        
        const size_t buffer_size = (n * sizeof(T)) / sizeof(uint8_t);
        sparrow::buffer<uint8_t> b(buffer_size);
        std::fill_n(b.data<T>(), n, value);
        ad.buffers.push_back(b);
        
        m_data = std::move(ad);
        m_layout.rebind_data(m_data);
    }

    // TODO: see comment above declaration of these constructors.
    // fixed-layout non-nullable
    /*template <is_arrow_base_type T, arrow_layout L>
    template <class R>
        requires std::ranges::range<R>
            && std::convertible_to<std::ranges::range_value_t<R>, T> 
            && (!is_nullable_v<std::ranges::range_value_t<R>>)
            && mpl::is_type_instance_of_v<L, fixed_size_layout>
    typed_array_impl<T, L>::typed_array_impl(R&& range, D)
    {
        // num elements
        auto n = static_cast<size_t>(std::ranges::distance(range));

        // create the array_data object holding the data
        sparrow::array_data ad;
        ad.type = sparrow::data_descriptor(sparrow::arrow_traits<T>::type_id);
        ad.length = static_cast<typename array_data::length_type>(n);
        ad.offset = static_cast<std::int64_t>(0);
        ad.bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);

        //  the buffer holding the actual data
        const size_t buffer_size = (n * sizeof(T)) / sizeof(uint8_t);
        sparrow::buffer<uint8_t> b(buffer_size);

        // copy the range to the buffer
        std::ranges::copy(range, b.data<T>());

        // add the buffer to the array_data
        ad.buffers.push_back(b);

        // pass the data to the member variables
        m_data = std::move(ad);
        m_layout.rebind_data(m_data);
    }

    // fixed-layout nullable
    template <is_arrow_base_type T, arrow_layout L>
    template <class R>
        requires std::ranges::range<R>
            && is_nullable_of_convertible_to<std::ranges::range_value_t<R>, T>
            && mpl::is_type_instance_of_v<L, fixed_size_layout>
    typed_array_impl<T, L>::typed_array_impl(R&& range)
    {
        // num elements
        auto n = static_cast<size_t>(std::ranges::distance(range));

        // create the array_data object holding the data
        sparrow::array_data ad;
        ad.type = sparrow::data_descriptor(sparrow::arrow_traits<T>::type_id);
        ad.length = static_cast<typename array_data::length_type>(n);
        ad.offset = static_cast<std::int64_t>(0);
        ad.bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);

        const size_t buffer_size = (n * sizeof(T)) / sizeof(uint8_t);
        sparrow::buffer<uint8_t> b(buffer_size);

        // iterate over the range
        auto iter = b.data<T>();
        size_t index = 0;
        for (const auto& value : range)
        {
            if (value.has_value())
            {
                *iter = value.value();
            }
            else
            {
                ad.bitmap.set(index, false);
            }
            ++iter;
            ++index;
        }

        // add the buffer to the array_data
        ad.buffers.push_back(b);

        // pass the data to the member variables
        m_data = std::move(ad);
        m_layout.rebind_data(m_data);
    }

    // variable-sized-layout non-nullable
    template <is_arrow_base_type T, arrow_layout L>
    template <class R>
        requires std::ranges::range<R>
            && std::convertible_to<std::ranges::range_value_t<R>, T>
            && (!is_nullable_v<std::ranges::range_value_t<R>>)
            && mpl::is_type_instance_of_v<L, variable_size_binary_layout>
    typed_array_impl<T, L>::typed_array_impl(R&& words_range)
    {
        // num elements
        auto n = static_cast<size_t>(std::ranges::distance(words_range));

        // create the array_data object holding the data
        sparrow::array_data ad;
        ad.type = sparrow::data_descriptor(sparrow::arrow_traits<T>::type_id);
        ad.length = static_cast<typename array_data::length_type>(n);
        ad.offset = static_cast<std::int64_t>(0);
        ad.bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);
        ad.buffers.resize(2);

        // offsets
        ad.buffers[0].resize(sizeof(std::int64_t) * (n + 1));

        // total size of the buffer holding the elements
        ad.buffers[1].resize(std::accumulate(
            words_range.begin(), words_range.end(), 
            size_t(0),
            [](std::size_t res, const auto& s)
            {
                return res + s.size();
            }
        ));

        auto offset_ptr = ad.buffers[0].data<std::int64_t>();

        //iterate over the words and copy them to the buffer
        auto iter = ad.buffers[1].begin();

        // offset at 0 is 0
        offset_ptr[0] = 0u;

        auto word_index = 0;
        for(auto word : words_range)
        {
            const auto word_size = static_cast<sparrow::array_data::buffer_type::difference_type>(word.size());
            offset_ptr[word_index + 1] = offset_ptr[word_index] + word_size;
            std::ranges::copy(word, iter);
            iter += word_size;
            ++word_index;
        }
    
        // pass the data to the member variables
        m_data = std::move(ad);
        m_layout.rebind_data(m_data);
    }

    // variable-sized-layout nullable
    template <is_arrow_base_type T, arrow_layout L>
    template <class R>
    requires std::ranges::range<R>
        && is_nullable_of_convertible_to<std::ranges::range_value_t<R>, T>
        && mpl::is_type_instance_of_v<L, variable_size_binary_layout>
    typed_array_impl<T, L>::typed_array_impl(R&& words_range)
    {
        // num elements
        auto n = static_cast<size_t>(std::ranges::distance(words_range));

        // create the array_data object holding the data
        sparrow::array_data ad;
        ad.type = sparrow::data_descriptor(sparrow::arrow_traits<T>::type_id);
        ad.length = static_cast<typename array_data::length_type>(n);
        ad.offset = static_cast<std::int64_t>(0);
        ad.bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);
        ad.buffers.resize(2);

        // offsets
        ad.buffers[0].resize(sizeof(std::int64_t) * (n + 1));

        // total size of the buffer holding the elements
        ad.buffers[1].resize(std::accumulate(
            words_range.begin(), words_range.end(), 
            size_t(0),
            [](std::size_t res, const auto& s)
            {
                if(s.has_value())
                {
                    return res + s.value().size();
                }
                return res;
            }
        ));

        auto offset_ptr = ad.buffers[0].data<std::int64_t>();

        //iterate over the words and copy them to the buffer
        auto iter = ad.buffers[1].begin();

        // offset at 0 is 0
        offset_ptr[0] = 0u;

        size_t word_index = 0;
        for(auto maybe_word : words_range)
        {
            if(maybe_word.has_value())
            {
                const auto & word = maybe_word.value();
                const auto word_size = static_cast<sparrow::array_data::buffer_type::difference_type>(word.size());
                offset_ptr[word_index + 1] = offset_ptr[word_index] + word_size;
                std::ranges::copy(word, iter);
                iter += word_size;
            }
            else{
                offset_ptr[word_index + 1] = offset_ptr[word_index];
                ad.bitmap.set(word_index, false);
            }
            ++word_index;
        }
    
        // pass the data to the member variables
        m_data = std::move(ad);
        m_layout.rebind_data(m_data);
    }*/

    // Value semantics

    template <is_arrow_base_type T, arrow_layout L>
    typed_array_impl<T, L>::typed_array_impl(const typed_array_impl& rhs)
        : m_data(rhs.m_data)
        , m_layout(m_data)
    {
    }

    template <is_arrow_base_type T, arrow_layout L>
    typed_array_impl<T, L>::typed_array_impl(typed_array_impl&& rhs)
        : m_data(std::move(rhs.m_data))
        , m_layout(m_data)
    {
    }

    template <is_arrow_base_type T, arrow_layout L>
    typed_array_impl<T, L>& typed_array_impl<T, L>::operator=(const typed_array_impl& rhs)
    {
        m_data = rhs.m_data;
        m_layout.rebind_data(m_data);
        return *this;
    }

    template <is_arrow_base_type T, arrow_layout L>
    typed_array_impl<T, L>& typed_array_impl<T, L>::operator=(typed_array_impl&& rhs)
    {
        m_data = std::move(rhs.m_data);
        m_layout.rebind_data(m_data);
        return *this;
    }

    // Element access

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::at(size_type i) -> reference
    {
        if (i >= size())
        {
            // TODO: Use our own format function
            throw std::out_of_range(
                "typed_array_impl::at: index out of range for array of size " + std::to_string(size())
                + " at index " + std::to_string(i)
            );
        }
        return m_layout[i];
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::at(size_type i) const -> const_reference
    {
        if (i >= size())
        {
            // TODO: Use our own format function
            throw std::out_of_range(
                "typed_array_impl::at: index out of range for array of size " + std::to_string(size())
                + " at index " + std::to_string(i)
            );
        }
        return m_layout[i];
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::operator[](size_type i) -> reference
    {
        SPARROW_ASSERT_TRUE(i < size())
        return m_layout[i];
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::operator[](size_type i) const -> const_reference
    {
        SPARROW_ASSERT_TRUE(i < size())
        return m_layout[i];
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::front() -> reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return m_layout[0];
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::front() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return m_layout[0];
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::back() -> reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return m_layout[size() - 1];
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::back() const -> const_reference
    {
        SPARROW_ASSERT_FALSE(empty());
        return m_layout[size() - 1];
    }

    // Iterators

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::begin() -> iterator
    {
        return m_layout.begin();
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::begin() const -> const_iterator
    {
        return m_layout.cbegin();
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::end() -> iterator
    {
        return m_layout.end();
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::end() const -> const_iterator
    {
        return m_layout.cend();
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::cbegin() const -> const_iterator
    {
        return begin();
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::cend() const -> const_iterator
    {
        return end();
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::bitmap() const -> const_bitmap_range
    {
        return m_layout.bitmap();
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::values() const -> const_value_range
    {
        return m_layout.values();
    }

    // Capacity

    template <is_arrow_base_type T, arrow_layout L>
    bool typed_array_impl<T, L>::empty() const
    {
        return m_layout.size() == 0;
    }

    template <is_arrow_base_type T, arrow_layout L>
    auto typed_array_impl<T, L>::size() const -> size_type
    {
        return m_layout.size();
    }

    // Comparators

    template <class T, class Layout>
    std::partial_ordering
    operator<=>(const typed_array_impl<T, Layout>& ta1, const typed_array_impl<T, Layout>& ta2)
    {
        return lexicographical_compare_three_way(ta1, ta2);
    }

    template <class T, class Layout>
    bool operator==(const typed_array_impl<T, Layout>& ta1, const typed_array_impl<T, Layout>& ta2)
    {   
        // see https://github.com/man-group/sparrow/issues/108
#if defined(_LIBCPP_VERSION) && (_LIBCPP_VERSION < 190000)
        if(ta1.size() != ta2.size())
        { 
            return false;
        }
        auto first1 = ta1.cbegin();
        auto last1 = ta1.cend();
        auto first2 = ta2.cbegin();
        for(; first1 != last1; ++first1, ++first2){
            if (!(*first1 == *first2)){
                return false;
            }
        }
        return true;
#else
        return std::equal(ta1.cbegin(), ta1.cend(), ta2.cbegin(), ta2.cend());
#endif
    }

}  // namespace sparrow
