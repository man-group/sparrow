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

        array_base(array_proxy proxy);
        array_base(const array_base&) = default;

        const array_proxy& data() const;

    private:

        virtual array_base* clone_impl() const = 0;

        array_proxy m_proxy;
    };

    /**
     * Base class for arrays with bitmap
     */
    class array_with_bitmap : public array_base
    {
    public:

        using bitmap_type = dynamic_bitset_view<std::int8_t>;

        virtual ~array_with_bitmap() = default;

    protected:

        array_with_bitmap(array_proxy proxy);
        array_with_bitmap(const array_with_bitmap&);

        const bitmap_type& bitmap() const;

    private:

        bitmap_type m_bitmap;
    };
}
