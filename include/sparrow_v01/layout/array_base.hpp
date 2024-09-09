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

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/buffer/dynamic_bitset.hpp"

namespace sparrow
{
    /**
     * Base class for array type erasure
     */
    class array_base
    {
    public:

        virtual ~array_base() = default;

        array_base(array_base&&) = delete;
        array_base& operator=(const array_base&) = delete;
        array_base& operator=(array_base&&) = delete;

        array_base* clone() const;

    protected:

        array_base(arrow_proxy proxy);
        array_base(const array_base&) = default;

        const arrow_proxy& data() const;

    private:

        virtual array_base* clone_impl() const = 0;

        arrow_proxy m_proxy;
    };

    /**
     * Base class for arrays with bitmap
     */
    class array_with_bitmap : public array_base
    {
    public:

        using bitmap_type = dynamic_bitset_view<const std::uint8_t>;

        virtual ~array_with_bitmap() = default;

    protected:

        array_with_bitmap(arrow_proxy proxy);
        array_with_bitmap(const array_with_bitmap&);

        const bitmap_type& bitmap() const;

    private:

        bitmap_type init_bitmap() const;
        bitmap_type m_bitmap;
    };

    /*****************************
     * array_base implementation *
     *****************************/

    inline array_base* array_base::clone() const
    {
        return clone_impl();
    }

    inline array_base::array_base(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
    {
    }

    inline const arrow_proxy& array_base::data() const
    {
        return m_proxy;
    }

    /************************************
     * array_with_bitmap implementation *
     ************************************/

    inline array_with_bitmap::array_with_bitmap(arrow_proxy proxy)
        : array_base(std::move(proxy))
        , m_bitmap(init_bitmap())
    {
    }


    inline array_with_bitmap::array_with_bitmap(const array_with_bitmap& rhs)
        : array_base(rhs)
        , m_bitmap(init_bitmap())
    {
    }

    inline auto array_with_bitmap::bitmap() const -> const bitmap_type&
    {
        return m_bitmap;
    }

    inline auto array_with_bitmap::init_bitmap() const -> bitmap_type
    {
        return bitmap_type(array_base::data().buffers()[0].data(),
                           array_base::data().buffers()[0].size());
    }
}
