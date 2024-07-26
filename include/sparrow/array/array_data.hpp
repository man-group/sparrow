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

#include <compare>
#include <optional>
#include <vector>

#include "sparrow/c_interface.hpp"
#include "sparrow/array/data_type.hpp"
#include "sparrow/buffer/buffer.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{
    /**
     * Structure holding the raw data.
     *
     * array_data is meant to be used by the
     * different layout classes to implement
     * the array API, based on the type
     * specified in the type attribute.
     */
    struct array_data
    {
        using block_type = std::uint8_t;
        using bitmap_type = dynamic_bitset<block_type>;
        using buffer_type = buffer<block_type>;
        using length_type = std::int64_t;

        data_descriptor type;
        length_type length = 0;
        std::int64_t offset = 0;
        // bitmap buffer and null_count
        bitmap_type bitmap;
        // Other buffers
        std::vector<buffer_type> buffers;
        std::vector<array_data> child_data;
        value_ptr<array_data> dictionary;
    };

    data_descriptor type_descriptor(const array_data& data);
    array_data::length_type length(const array_data& data);
    std::int64_t offset(const array_data& data);

    array_data::bitmap_type& bitmap(array_data& data);
    const array_data::bitmap_type& bitmap(const array_data& data);

    std::size_t buffers_size(const array_data& data);
    array_data::buffer_type& buffer_at(array_data& data, std::size_t i);
    const array_data::buffer_type& buffer_at(const array_data& data, std::size_t i);

    std::size_t child_data_size(const array_data& data);
    array_data& child_data_at(array_data& data, std::size_t i);
    const array_data& child_data_at(const array_data& data, std::size_t i);

    value_ptr<array_data>& dictionary(array_data& data);
    const value_ptr<array_data>& dictionary(const array_data& data);

    /**
     * Internal wrapper for external_array_data
     */
    namespace impl
    {
        template <class T>
        class external_wrapper
        {
        public:

            using self_type = external_wrapper<T>;

            template <class U>
            requires std::is_rvalue_reference_v<U&&>
            external_wrapper(U&&, bool own);

            template <class U>
            requires std::is_pointer_v<U>
            external_wrapper(U, bool own);

            T& data();
            const T& data() const;

        private:

            struct no_releaser
            {
                void operator()(T*) const {}
            };

            struct releaser
            {
                void operator()(T* t) const;
            };

            std::shared_ptr<T> p_data = nullptr;
        };

        template <class T>
        constexpr bool is_arrow_schema_v = std::same_as<T, ArrowSchema> 
                                        or std::same_as<std::remove_reference_t<T>, ArrowSchema*>;

        template <class T>
        constexpr bool is_arrow_array_v = std::same_as<T, ArrowArray>
                                       or std::same_as<std::remove_reference_t<T>, ArrowArray*>;
    }

    /**
     * Structure holding raw data allocated outside of sparrow
     */
    class external_array_data
    {
    public:

        using block_type = std::uint8_t;
        using bitmap_type = dynamic_bitset_view<const block_type>;
        using buffer_type = buffer_view<const block_type>;
        using length_type = std::int64_t;

        template <class S, class A>
        requires impl::is_arrow_schema_v<S> and impl::is_arrow_array_v<A>
        external_array_data(S&& os, bool own_schema, A&& oa, bool own_array);

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

    /***********************************
     * getter functions for array_data *
     ***********************************/

    inline data_descriptor type_descriptor(const array_data& data)
    {
        return data.type;
    }

    inline array_data::length_type length(const array_data& data)
    {
        return data.length;
    }

    inline std::int64_t offset(const array_data& data)
    {
        return data.offset;
    }

    inline array_data::bitmap_type& bitmap(array_data& data)
    {
        return data.bitmap;
    }

    inline const array_data::bitmap_type& bitmap(const array_data& data)
    {
        return data.bitmap;
    }

    inline std::size_t buffers_size(const array_data& data)
    {
        return data.buffers.size();
    }

    inline array_data::buffer_type& buffer_at(array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < buffers_size(data));
        return data.buffers[i];
    }

    inline const array_data::buffer_type& buffer_at(const array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < buffers_size(data));
        return data.buffers[i];
    }

    inline std::size_t child_data_size(const array_data& data)
    {
        return data.child_data.size();
    }

    inline array_data& child_data_at(array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < child_data_size(data));
        return data.child_data[i];
    }

    inline const array_data& child_data_at(const array_data& data, std::size_t i)
    {
        SPARROW_ASSERT_TRUE(i < child_data_size(data));
        return data.child_data[i];
    }

    inline value_ptr<array_data>& dictionary(array_data& data)
    {
        return data.dictionary;
    }

    inline const value_ptr<array_data>& dictionary(const array_data& data)
    {
        return data.dictionary;
    }

    /***********************************
     * external_wrapper implementation *
     ***********************************/

    namespace impl
    {
        template <class T>
        template <class U>
        requires std::is_rvalue_reference_v<U&&>
        external_wrapper<T>::external_wrapper(U&& u, bool own)
        {
            if (own)
            {
                auto l = [r = releaser()](T* t)
                {
                    r(t);
                    delete t;
                };
                p_data = std::shared_ptr<T>(new T(std::move(u)), std::move(l));
            }
            else
            {
                p_data = std::shared_ptr<T>(new T(std::move(u)));
            }
        }

        template <class T>
        template <class U>
        requires std::is_pointer_v<U>
        external_wrapper<T>::external_wrapper(U u, bool own)
        {
            if (own)
            {
                p_data = std::shared_ptr<T>(u, releaser());
            }
            else
            {
                p_data = std::shared_ptr<T>(u, no_releaser());
            }
        }

        template <class T>
        T& external_wrapper<T>::data()
        {
            return *p_data;
        }

        template <class T>
        const T& external_wrapper<T>::data() const
        {
            return *p_data;
        }

        template <class T>
        void external_wrapper<T>::releaser::operator()(T* t) const
        {
            if (t->release)
            {
                t->release(t);
                t->release = nullptr;
            }
        }
    }

    /**************************************
     * external_array_data implementation *
     **************************************/
    
    template <class S, class A>
    requires impl::is_arrow_schema_v<S> and impl::is_arrow_array_v<A>
    external_array_data::external_array_data(S&& os, bool own_schema, A&& oa, bool own_array)
        : m_schema(std::forward<S>(os), own_schema)
        , m_array(std::forward<A>(oa), own_array)
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
        auto size = static_cast<std::size_t>(array().n_children);
        m_children.reserve(size);
        for (std::size_t i = 0; i < size; ++i)
        {
            external_array_data tmp(schema().children[i], false, array().children[i], false);
            m_children.push_back(std::move(tmp));
            //m_children.emplace_back(schema().children[i], false, array().children[i], false);
        }
    }

    inline void external_array_data::build_dictionary()
    {
        if (schema().dictionary && array().dictionary)
        {
            m_dictionary = value_ptr(external_array_data(schema().dictionary, false, array().dictionary, false));
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
                           static_cast<std::size_t>(length(data)));
    }

    inline std::size_t buffers_size(const external_array_data& data)
    {
        // Special case: the null_layout does not allocate any buffer
        if (data.array().n_buffers == 0)
        {
            return std::size_t(0);
        }
        // The first buffer in external data is used for the bitmap
        return static_cast<std::size_t>(data.array().n_buffers - 1);
    }

    inline external_array_data::buffer_type
    buffer_at(const external_array_data& data, std::size_t i)
    {
        using return_type = external_array_data::buffer_type;
        // The first buffer in exyternal data is used for the bitmap
        return return_type(impl::buffer_at(data, i + 1u),
                           static_cast<std::size_t>(length(data)));
    }

    inline std::size_t child_data_size(const external_array_data& data)
    {
        return static_cast<std::size_t>(data.array().n_children);
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
