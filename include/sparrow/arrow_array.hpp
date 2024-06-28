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

#include <cstddef>
#include <cstdint>
#include <utility>

#include "sparrow/any_data.hpp"
#include "sparrow/arrow_array_schema_utils.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/data_type.hpp"
#include "sparrow/mp_utils.hpp"

namespace sparrow
{
    inline void arrow_array_custom_deleter(ArrowArray* array)
    {
        if (array == nullptr)
        {
            return;
        }
        if (array->release != nullptr)
        {
            array->release(array);
        }
        delete array;
    }

    /**
     * Custom deleter for ArrowArray.
     */
    struct arrow_array_custom_deleter_struct
    {
        void operator()(ArrowArray* array) const
        {
            arrow_array_custom_deleter(array);
        }
    };

    /// Unique pointer to an ArrowArray. Must be used to manage the memory of an ArrowArray.
    using arrow_array_unique_ptr = std::unique_ptr<ArrowArray, arrow_array_custom_deleter_struct>;

    /// Shared pointer to an ArrowArray. Must be used to manage the memory of an ArrowArray.
    class arrow_array_shared_ptr : public std::shared_ptr<ArrowArray>
    {
    public:

        explicit arrow_array_shared_ptr()
            : std::shared_ptr<ArrowArray>(nullptr, arrow_array_custom_deleter)
        {
        }

        explicit arrow_array_shared_ptr(arrow_array_unique_ptr&& ptr)
            : std::shared_ptr<ArrowArray>(std::move(ptr))
        {
        }

        explicit arrow_array_shared_ptr(std::nullptr_t) noexcept
            : std::shared_ptr<ArrowArray>(nullptr, arrow_array_custom_deleter)
        {
        }
    };

    template <class T>
    concept valid_type_for_arrow_array_buffer = (std::ranges::input_range<T>
                                                 && sparrow::is_arrow_base_type<std::ranges::range_value_t<T>>)
                                                || (std::is_pointer_v<T>
                                                    && (sparrow::is_arrow_base_type<std::remove_pointer_t<T>>) )
                                                || (mpl::smart_ptr_and_derived<T>
                                                    && (sparrow::is_arrow_base_type<typename T::element_type>) )
                                                || ((mpl::is_type_instance_of_v<T, sparrow::value_ptr>)
                                                    && (sparrow::is_arrow_base_type<typename T::element_type>) );

    struct
    {
        template <class T>
        consteval bool operator()(mpl::typelist<T>)
        {
            return valid_type_for_arrow_array_buffer<T>;
        }
    } constexpr valid_type_for_arrow_array_buffer_predicate;

    template <class Tuple>
    concept valid_type_for_buffer_tuple = mpl::is_type_instance_of_v<Tuple, std::tuple>
                                          && mpl::all_of(
                                              mpl::rename<Tuple, mpl::typelist>{},
                                              valid_type_for_arrow_array_buffer_predicate
                                          );

    /**
     * Struct representing private data for ArrowArray.
     *
     * This struct holds the private data for ArrowArray, including buffers, children, and dictionary.
     * It is used in the Sparrow library.
     *
     * @tparam BufferType The type of the buffers.
     */
    template <class BufferType>
    struct arrow_array_private_data
    {
        template <typename B, typename C, typename D>
            requires(arrow_array_buffers_or_children<B> || valid_type_for_buffer_tuple<B>)
                    && arrow_array_buffers_or_children<C>
        explicit arrow_array_private_data(B buffers, C children, D dictionary);

        [[nodiscard]] any_data_container<BufferType>& buffers() noexcept;

        [[nodiscard]] const BufferType** buffers_ptrs() noexcept;

        [[nodiscard]] any_data_container<ArrowArray>& children() noexcept;

        [[nodiscard]] ArrowArray** children_ptrs() noexcept;

        [[nodiscard]] any_data<ArrowArray>& dictionary() noexcept;

        [[nodiscard]] ArrowArray* dictionary_ptr() noexcept;

    private:

        any_data_container<BufferType> m_buffers;
        any_data_container<ArrowArray> m_children;
        any_data<ArrowArray> m_dictionary;
    };

    template <class BufferType>
    template <typename B, typename C, typename D>
        requires(arrow_array_buffers_or_children<B> || valid_type_for_buffer_tuple<B>)
                    && arrow_array_buffers_or_children<C>
    arrow_array_private_data<BufferType>::arrow_array_private_data(B buffers, C children, D dictionary)
        : m_buffers(std::move(buffers))
        , m_children(std::move(children))
        , m_dictionary(std::move(dictionary))
    {
    }

    template <class BufferType>
    [[nodiscard]] any_data_container<BufferType>& arrow_array_private_data<BufferType>::buffers() noexcept
    {
        return m_buffers;
    }

    template <class BufferType>
    [[nodiscard]] const BufferType** arrow_array_private_data<BufferType>::buffers_ptrs() noexcept
    {
        return const_cast<const BufferType**>(m_buffers.get());
    }

    template <class BufferType>
    [[nodiscard]] any_data_container<ArrowArray>& arrow_array_private_data<BufferType>::children() noexcept
    {
        return m_children;
    }

    template <class BufferType>
    [[nodiscard]] ArrowArray** arrow_array_private_data<BufferType>::children_ptrs() noexcept
    {
        return m_children.get();
    }

    template <class BufferType>
    [[nodiscard]] any_data<ArrowArray>& arrow_array_private_data<BufferType>::dictionary() noexcept
    {
        return m_dictionary;
    }

    template <class BufferType>
    [[nodiscard]] ArrowArray* arrow_array_private_data<BufferType>::dictionary_ptr() noexcept
    {
        return m_dictionary.get();
    }

    /**
     * Deletes an ArrowArray.
     *
     * @tparam T The type of the buffers of the ArrowArray.
     * @param array The ArrowArray to delete. Should be a valid pointer to an ArrowArray. Its release callback
     * should be set.
     */
    template <typename T>
    void delete_array(ArrowArray* array)
    {
        SPARROW_ASSERT_FALSE(array == nullptr)

        array->buffers = nullptr;
        array->n_buffers = 0;
        array->length = 0;
        array->null_count = 0;
        array->offset = 0;
        array->n_children = 0;
        array->children = nullptr;
        array->dictionary = nullptr;
        if (array->private_data != nullptr)
        {
            const auto private_data = static_cast<arrow_array_private_data<T>*>(array->private_data);
            delete private_data;
        }
        array->private_data = nullptr;
        array->release = nullptr;
    }

    inline arrow_array_unique_ptr default_arrow_array()
    {
        auto ptr = arrow_array_unique_ptr(new ArrowArray());
        ptr->length = 0;
        ptr->null_count = 0;
        ptr->offset = 0;
        ptr->n_buffers = 0;
        ptr->n_children = 0;
        ptr->buffers = nullptr;
        ptr->children = nullptr;
        ptr->dictionary = nullptr;
        ptr->release = nullptr;
        ptr->private_data = nullptr;
        return ptr;
    }

    // TODO: Auto deduction of T from B
    template <class T, class B, class C, class D>
        requires arrow_array_buffers_or_children<B> && arrow_array_buffers_or_children<C>
    arrow_array_unique_ptr
    make_arrow_array(int64_t length, int64_t null_count, int64_t offset, B buffers, C children, D dictionary);

    /// Creates an ArrowArray.
    ///
    /// @tparam T The type of the buffers of the ArrowArray
    /// @tparam B The type of the buffers of the ArrowArray. Can be a range/tuples/raw pointers/smart
    /// pointers.
    ///           Example: TODO
    /// @tparam C The type of the children of the ArrowArray. Can be a range/tuples/raw pointers/smart
    /// pointers.
    /// @tparam D The type of the dictionary of the ArrowArray. Can be a raw pointer, smart pointer or an
    /// object
    /// @param length The logical length of the array (i.e. its number of items). Must be 0 or positive.
    /// @param null_count The number of null items in the array. May be -1 if not yet computed. Must be 0 or
    /// positive otherwise.
    /// @param offset The logical offset inside the array (i.e. the number of items from the physical start of
    ///               the buffers). Must be 0 or positive.
    /// @param n_buffers The number of physical buffers backing this array. The number of buffers is a
    ///                  function of the data type, as described in the Columnar format specification, except
    ///                  for the the binary or utf-8 view type, which has one additional buffer compared to
    ///                  the Columnar format specification (see Binary view arrays). Must be 0 or positive.
    /// @param children Vector of child arrays. Children must not be null.
    /// @param dictionary A pointer to the underlying array of dictionary values. Must be present if the
    ///                   ArrowArray represents a dictionary-encoded array. Must be null otherwise.
    /// @return The created ArrowArray.
    /// TODO: Auto deduction of T from B
    template <class T, class B, class C, class D>
    arrow_array_unique_ptr make_arrow_array(
        int64_t length,
        int64_t null_count,
        int64_t offset,
        int64_t n_buffers,
        B buffers,
        int64_t n_children,
        C children,
        D dictionary
    )
    {
        SPARROW_ASSERT_TRUE(length >= 0);
        SPARROW_ASSERT_TRUE(null_count >= -1);
        SPARROW_ASSERT_TRUE(offset >= 0);
        SPARROW_ASSERT_TRUE(n_buffers >= 0);
        SPARROW_ASSERT_TRUE(n_children >= 0);

        arrow_array_unique_ptr array = default_arrow_array();

        array->length = length;
        array->null_count = null_count;
        array->offset = offset;
        array->n_buffers = n_buffers;

        array->private_data = new arrow_array_private_data<T>(
            std::move(buffers),
            std::move(children),
            std::move(dictionary)
        );
        const auto private_data = static_cast<arrow_array_private_data<T>*>(array->private_data);
        array->buffers = reinterpret_cast<const void**>(private_data->buffers_ptrs());
        array->n_children = n_children;
        array->children = private_data->children_ptrs();
        array->dictionary = private_data->dictionary_ptr();
        array->release = delete_array<T>;
        return array;
    }

    // TODO: Auto deduction of T from B
    template <class T, class B, class C, class D>
        requires arrow_array_buffers_or_children<B> && arrow_array_buffers_or_children<C>
    arrow_array_unique_ptr
    make_arrow_array(int64_t length, int64_t null_count, int64_t offset, B buffers, C children, D dictionary)
    {
        const int64_t buffer_count = get_size(buffers);
        const int64_t children_count = get_size(children);
        return make_arrow_array<T, B, C, D>(
            length,
            null_count,
            offset,
            buffer_count,
            std::move(buffers),
            children_count,
            std::move(children),
            std::move(dictionary)
        );
    }
}
