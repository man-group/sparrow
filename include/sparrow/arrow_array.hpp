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
#include <type_traits>
#include <utility>

#include "sparrow/arrow_array_schema_utils.hpp"
#include "sparrow/buffer.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/contracts.hpp"
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

        arrow_array_shared_ptr()
            : std::shared_ptr<ArrowArray>(nullptr, arrow_array_custom_deleter)
        {
        }

        arrow_array_shared_ptr(arrow_array_unique_ptr&& ptr)
            : std::shared_ptr<ArrowArray>(std::move(ptr))
        {
        }

        arrow_array_shared_ptr(std::nullptr_t) noexcept
            : std::shared_ptr<ArrowArray>(nullptr, arrow_array_custom_deleter)
        {
        }
    };

    /**
     * Struct representing private data for ArrowArray.
     *
     * This struct holds the private data for ArrowArray, including buffers, children, and dictionary.
     * It is used in the Sparrow library.
     */
    struct arrow_array_private_data
    {
        arrow_array_private_data(
            std::vector<buffer<std::uint8_t>> buffers,
            std::vector<arrow_array_shared_ptr> children,
            arrow_array_shared_ptr dictionary
        );

        [[nodiscard]] const std::vector<buffer<uint8_t>>& buffers() const noexcept;

        template <class T>
        [[nodiscard]] const T** buffers_ptrs() noexcept;

        [[nodiscard]] const std::vector<arrow_array_shared_ptr>& children() const noexcept;

        [[nodiscard]] ArrowArray** children_ptrs() noexcept;

        [[nodiscard]] const arrow_array_shared_ptr& dictionary() const noexcept;

        [[nodiscard]] ArrowArray* dictionary_ptr() noexcept;

    private:

        std::vector<buffer<std::uint8_t>> m_buffers;
        std::vector<std::uint8_t*> m_buffers_pointers;
        std::vector<arrow_array_shared_ptr> m_children;
        std::vector<ArrowArray*> m_children_pointers;
        arrow_array_shared_ptr m_dictionary;
    };

    arrow_array_private_data::arrow_array_private_data(
        std::vector<buffer<std::uint8_t>> buffers,
        std::vector<arrow_array_shared_ptr> children,
        arrow_array_shared_ptr dictionary
    )
        : m_buffers(std::move(buffers))
        , m_buffers_pointers(to_raw_ptr_vec<std::uint8_t>(m_buffers))
        , m_children(std::move(children))
        , m_children_pointers(to_raw_ptr_vec<ArrowArray>(m_children))
        , m_dictionary(std::move(dictionary))
    {
    }

    [[nodiscard]] inline const std::vector<buffer<std::uint8_t>>&
    arrow_array_private_data::buffers() const noexcept
    {
        return m_buffers;
    }

    template <class T>
    [[nodiscard]] const T** arrow_array_private_data::buffers_ptrs() noexcept
    {
        return const_cast<const T**>(reinterpret_cast<T**>(m_buffers_pointers.data()));
    }

    [[nodiscard]] inline const std::vector<arrow_array_shared_ptr>&
    arrow_array_private_data::children() const noexcept
    {
        return m_children;
    }

    [[nodiscard]] inline ArrowArray** arrow_array_private_data::children_ptrs() noexcept
    {
        if (m_children_pointers.empty())
        {
            return nullptr;
        }
        return m_children_pointers.data();
    }

    [[nodiscard]] inline const arrow_array_shared_ptr& arrow_array_private_data::dictionary() const noexcept
    {
        return m_dictionary;
    }

    [[nodiscard]] inline ArrowArray* arrow_array_private_data::dictionary_ptr() noexcept
    {
        return m_dictionary.get();
    }

    /**
     * Deletes an ArrowArray.
     *
     * @param array The ArrowArray to delete. Should be a valid pointer to an ArrowArray. Its release callback
     * should be set.
     */
    inline void delete_array(ArrowArray* array)
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
            const auto private_data = static_cast<arrow_array_private_data*>(array->private_data);
            delete private_data;
        }
        array->private_data = nullptr;
        array->release = nullptr;
    }

    inline arrow_array_unique_ptr default_arrow_array_unique_ptr()
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

    template <class B, class C, class D>
    arrow_array_unique_ptr
    make_arrow_array_unique_ptr(int64_t length, int64_t null_count, int64_t offset, B buffers, C children, D dictionary);

    /// Creates an ArrowArray.
    ///
    /// @tparam B Value, reference or rvalue of std::vector<sparrow::buffer<uint8_t>>
    /// @tparam C Value, reference or rvalue of std::vector<arrow_array_shared_ptr>
    /// @tparam D Value, reference or rvalue of arrow_array_shared_ptr
    /// @param length The logical length of the array (i.e. its number of items). Must be 0 or positive.
    /// @param null_count The number of null items in the array. May be -1 if not yet computed. Must be 0 or
    /// positive otherwise.
    /// @param offset The logical offset inside the array (i.e. the number of items from the physical start of
    ///               the buffers). Must be 0 or positive.
    /// @param n_buffers The number of physical buffers backing this array. The number of buffers is a
    ///                  function of the data type, as described in the Columnar format specification, except
    ///                  for the the binary or utf-8 view type, which has one additional buffer compared to
    ///                  the Columnar format specification (see Binary view arrays). Must be 0 or positive.
    /// @param buffers Vector of sparrow::buffer
    /// @param children Vector of arrow_array_shared_ptr representing the children of the ArrowArray.
    /// @param dictionary arrow_array_shared_ptr or nullptr.
    /// @return The created ArrowArray.
    template <class B, class C, class D>
        requires mpl::is_same_relaxed<B, std::vector<buffer<std::uint8_t>>>
                 && mpl::is_same_relaxed<C, std::vector<arrow_array_shared_ptr>>
                 && mpl::is_same_relaxed<D, arrow_array_shared_ptr>
    arrow_array_unique_ptr make_arrow_array_unique_ptr(
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

        arrow_array_unique_ptr array = default_arrow_array_unique_ptr();

        array->length = length;
        array->null_count = null_count;
        array->offset = offset;
        array->n_buffers = n_buffers;

        array->private_data = new arrow_array_private_data(
            std::move(buffers),
            std::move(children),
            std::move(dictionary)
        );
        const auto private_data = static_cast<arrow_array_private_data*>(array->private_data);
        array->buffers = private_data->buffers_ptrs<void>();
        array->n_children = n_children;
        array->children = private_data->children_ptrs();
        array->dictionary = private_data->dictionary_ptr();
        array->release = delete_array;
        return array;
    }

    template <class B, class C, class D>
    arrow_array_unique_ptr
    make_arrow_array_unique_ptr(int64_t length, int64_t null_count, int64_t offset, B buffers, C children, D dictionary)
    {
        const int64_t buffer_count = get_size(buffers);
        const int64_t children_count = get_size(children);
        return make_arrow_array_unique_ptr<B, C, D>(
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
