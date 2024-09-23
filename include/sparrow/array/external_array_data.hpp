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

#include "sparrow/c_interface.hpp"
#include "sparrow/array/array_data_concepts.hpp"
#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{

    /**
     * Internal wrapper for external_array_data
     */
    namespace impl
    {

        template <any_arrow_c_interface T>
        class external_wrapper
        {
        public:

            using self_type = external_wrapper<T>;

            template <class U>
                requires std::convertible_to<U, T>
            external_wrapper(U&&, ownership ownership_model);

            template <class U>
                requires std::is_pointer_v<std::remove_reference_t<U>> and std::convertible_to<U, T*>
            external_wrapper(U, ownership ownership_model);

            T& data();
            const T& data() const;

        private:

            // `shared_ptr` deleter that does nothing,
            // doesnt even delete the pointed structure.
            struct dont_release_anything
            {
                void operator()(T*) const {}
            };

            // `shared_ptr` deleter that only release
            // the Arrow data handled, not the pointed structure
            // that owns it.
            struct only_release_arrow_data
            {
                void operator()(T* t) const;
            };

            // `shared_ptr` deleter that releases both the
            // Arrow data handled and the pointed structure
            // that owns it.
            struct full_ownership_deleter
            {
                void operator()(T* t) const;
            };

            std::shared_ptr<T> p_data = nullptr;
        };


    }


    /**
     * Holds raw Arrow data allocated outside of this library.
     * Usually constructed using `ArrayArray` and `ArrowSchema` C structures
     * (see `arrow_interface/c_interface.hpp` for details).
     *
     * Data held by this type will not be modifiable but ownership will
     * be preserved according to the requested behavior specified at construction.
     *
     * This type is specifically designed to work as a `data_storage` usable
     * by layouts implementations. @see `data_storage` concept for details.
     *
     */
    class external_array_data
    {
    public:

        static constexpr bool is_mutable = false;
        using block_type = std::uint8_t;
        using bitmap_type = dynamic_bitset_view<const block_type>;
        using buffer_type = buffer_view<const block_type>;
        using length_type = std::int64_t;


        /**
         * Constructor acquiring data from `ArrowArray` and `ArrowSchema` C structures.
         * Ownership for either is specified through parameter `ownership`.
         * As per Arrow's format specification, if the data is own, the provided release functions
         * which are part of the provided structures will be used and must exist in that case.
         *
         * @param aschema `ArrowSchema` object passed by reference, value or pointer, that
         *                this object will handle.
         * @param aarray `ArrowArray` object passed by reference, value or pointer, that
         *                this object will handle.
         * @param ownership Specifies ownership of the data provided through the
                            `ArrowSchema` and `ArrowArray`.
         */
        template <arrow_schema_or_ptr S, arrow_array_or_ptr A>
        external_array_data(S&& aschema, A&& aarray, arrow_data_ownership ownership);

        const ArrowSchema& schema() const;
        const ArrowArray& array() const;

        const external_array_data& child_at(std::size_t i) const;
        const value_ptr<external_array_data>& dictionary() const;

    private:

        ArrowSchema& schema();
        ArrowArray& array();

        void build_children();
        void build_dictionary();

        impl::external_wrapper<ArrowSchema> m_schema;
        impl::external_wrapper<ArrowArray> m_array;

        std::vector<external_array_data> m_children;
        value_ptr<external_array_data> m_dictionary;
    };

    data_descriptor type_descriptor(const external_array_data& data);
    external_array_data::length_type length(const external_array_data& data);
    std::int64_t offset(const external_array_data& data);

    external_array_data::bitmap_type bitmap(const external_array_data& data);

    std::size_t buffers_size(const external_array_data& data);
    external_array_data::buffer_type buffer_at(const external_array_data& data, std::size_t i);

    std::size_t child_data_size(const external_array_data& data);
    const external_array_data& child_data_at(const external_array_data& data, std::size_t i);

    const value_ptr<external_array_data>& dictionary(const external_array_data& data);


    // `external_array_data` must always be usable as a data-storage that layout implementations can use.
    static_assert(data_storage<external_array_data>);


    /***********************************
     * external_wrapper implementation *
     ***********************************/

    namespace impl
    {
        template <any_arrow_c_interface T>
        template <class U>
            requires std::convertible_to<U, T>
        external_wrapper<T>::external_wrapper(U&& u, ownership ownership_model)
        {
            if (ownership_model == ownership::owning)
            {
                p_data = std::shared_ptr<T>(new T(std::forward<U>(u)), full_ownership_deleter{});
            }
            else
            {
                p_data = std::shared_ptr<T>(new T(std::forward<U>(u))); // default shared_ptr deleter
            }
        }

        template <any_arrow_c_interface T>
        template <class U>
            requires std::is_pointer_v<std::remove_reference_t<U>> and std::convertible_to<U, T*>
        external_wrapper<T>::external_wrapper(U ptr, ownership ownership_model)
        {
            if (ownership_model == ownership::owning)
            {
                p_data = std::shared_ptr<T>(ptr, only_release_arrow_data{});
            }
            else
            {
                p_data = std::shared_ptr<T>(ptr, dont_release_anything{});
            }
        }

        template <any_arrow_c_interface T>
        T& external_wrapper<T>::data()
        {
            return *p_data;
        }

        template <any_arrow_c_interface T>
        const T& external_wrapper<T>::data() const
        {
            return *p_data;
        }

        template <any_arrow_c_interface T>
        void external_wrapper<T>::only_release_arrow_data::operator()(T* t) const
        {
            if (t->release)
            {
                t->release(t); // implies `t->release = nullptr;`
            }
        }

        template <any_arrow_c_interface T>
        void external_wrapper<T>::full_ownership_deleter::operator()(T* t) const
        {
            if (t->release)
            {
                t->release(t); // implies `t->release = nullptr;`
            }
            delete t;
        }
    }

    /**************************************
     * external_array_data implementation *
     **************************************/

    template <arrow_schema_or_ptr S, arrow_array_or_ptr A>
    external_array_data::external_array_data(S&& aschema, A&& aarray, arrow_data_ownership ownership)
        : m_schema(std::forward<S>(aschema), ownership.schema)
        , m_array(std::forward<A>(aarray), ownership.array)
    {
        build_children();
        build_dictionary();
    }

    inline const ArrowSchema& external_array_data::schema() const
    {
        return m_schema.data();
    }

    inline const ArrowArray& external_array_data::array() const
    {
        return m_array.data();
    }

    inline const external_array_data& external_array_data::child_at(std::size_t i) const
    {
        return m_children[i];
    }

    inline const value_ptr<external_array_data>& external_array_data::dictionary() const
    {
        return m_dictionary;
    }

    inline ArrowSchema& external_array_data::schema()
    {
        return m_schema.data();
    }

    inline ArrowArray& external_array_data::array()
    {
        return m_array.data();
    }

    inline void external_array_data::build_children()
    {
        const auto size = to_native_size(array().n_children);
        m_children.reserve(size);
        for (std::size_t i = 0; i < size; ++i)
        {
            m_children.emplace_back(schema().children[i], array().children[i], doesnt_own_arrow_data);
        }
    }

    inline void external_array_data::build_dictionary()
    {
        if (schema().dictionary && array().dictionary)
        {
            m_dictionary = value_ptr(external_array_data(schema().dictionary, array().dictionary, doesnt_own_arrow_data));
        }
        else
        {
            m_dictionary = nullptr;
        }
    }

    /********************************************
     * getter functions for external_array_data *
     ********************************************/

    namespace impl
    {
        inline const external_array_data::block_type*
        buffer_at(const external_array_data& data, std::size_t i)
        {
            using block_type = external_array_data::block_type;
            return reinterpret_cast<const block_type*>(data.array().buffers[i]);
        }
    }

    inline data_descriptor type_descriptor(const external_array_data& data)
    {
        return data_descriptor(data.schema().format);
    }

    inline external_array_data::length_type
    length(const external_array_data& data)
    {
        return data.array().length;
    }

    inline std::int64_t offset(const external_array_data& data)
    {
        return data.array().offset;
    }

    inline external_array_data::bitmap_type
    bitmap(const external_array_data& data)
    {
        using return_type = external_array_data::bitmap_type;
        return return_type(impl::buffer_at(data, 0u),
                           to_native_size(length(data)));
    }

    inline std::size_t buffers_size(const external_array_data& data)
    {
        // Special case: the null_layout does not allocate any buffer
        if (data.array().n_buffers == 0)
        {
            return std::size_t(0);
        }
        // The first buffer in external data is used for the bitmap
        return sum_arrow_offsets<std::size_t>(data.array().n_buffers, - 1);
    }

    inline external_array_data::buffer_type
    buffer_at(const external_array_data& data, std::size_t i)
    {
        using return_type = external_array_data::buffer_type;
        // The first buffer in external data is used for the bitmap
        return return_type(impl::buffer_at(data, i + 1u),
                           to_native_size(length(data)));
    }

    inline std::size_t child_data_size(const external_array_data& data)
    {
        return to_native_size(data.array().n_children);
    }

    inline const external_array_data& child_data_at(const external_array_data& data, std::size_t i)
    {
        return data.child_at(i);
    }

    inline const value_ptr<external_array_data>& dictionary(const external_array_data& data)
    {
        return data.dictionary();
    }


}

