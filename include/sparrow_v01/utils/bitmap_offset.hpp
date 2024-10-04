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

#include <iterator>
#include <variant>

#include "sparrow/utils/iterator.hpp"


#pragma once

namespace sparrow
{

    template <typename Bitmap>
    class bitmap_offset
    {
    public:

        using size_type = Bitmap::size_type;
        using reference = Bitmap::reference;
        using const_reference = Bitmap::const_reference;
        using iterator = Bitmap::iterator;
        using const_iterator = Bitmap::const_iterator;

        explicit bitmap_offset(Bitmap&& bitmap, size_type offset);
        explicit bitmap_offset(Bitmap& bitmap, size_type offset);

        [[nodiscard]] size_type size() const;
        [[nodiscard]] bool empty() const;
        [[nodiscard]] size_type null_count() const;

        [[nodiscard]] bool test(size_type pos) const;

        [[nodiscard]] reference at(size_type pos);
        [[nodiscard]] const_reference at(size_type pos) const;

        [[nodiscard]] reference operator[](size_type i);
        [[nodiscard]] const_reference operator[](size_type i) const;

        [[nodiscard]] iterator begin();
        [[nodiscard]] iterator end();

        [[nodiscard]] const_iterator begin() const;
        [[nodiscard]] const_iterator end() const;
        [[nodiscard]] const_iterator cbegin() const;
        [[nodiscard]] const_iterator cend() const;

        [[nodiscard]] reference front();
        [[nodiscard]] const_reference front() const;
        [[nodiscard]] reference back();
        [[nodiscard]] const_reference back() const;

    private:

        Bitmap& bitmap();
        const Bitmap& bitmap() const;

        std::variant<Bitmap*, Bitmap> m_bitmap;
        size_type m_offset = 0;

        [[nodiscard]] size_t calculate_null_count() const;
        size_t m_null_count = 0;
    };

    /********************************
     * bitmap_offset implementation *
     ********************************/

    template <typename Bitmap>
    bitmap_offset<Bitmap>::bitmap_offset(Bitmap&& bitmap, size_type offset)
        : m_bitmap(std::move(bitmap))
        , m_offset(offset)
        , m_null_count(calculate_null_count())
    {
    }

    template <typename Bitmap>
    bitmap_offset<Bitmap>::bitmap_offset(Bitmap& bitmap, size_type offset)
        : m_bitmap(&bitmap)
        , m_offset(offset)
        , m_null_count(calculate_null_count())
    {
    }

    template <typename Bitmap>
    size_t bitmap_offset<Bitmap>::calculate_null_count() const
    {
        return static_cast<size_t>(std::count_if(
            cbegin(),
            cend(),
            [](const auto& value)
            {
                return !value;
            }
        ));
    }

    template <typename Bitmap>
    Bitmap& bitmap_offset<Bitmap>::bitmap()
    {
        return m_bitmap.index() == 0 ? *std::get<0>(m_bitmap) : std::get<1>(m_bitmap);
    }

    template <typename Bitmap>
    const Bitmap& bitmap_offset<Bitmap>::bitmap() const
    {
        return m_bitmap.index() == 0 ? *std::get<0>(m_bitmap) : std::get<1>(m_bitmap);
    }

    template <typename Bitmap>
    bitmap_offset<Bitmap>::size_type bitmap_offset<Bitmap>::size() const
    {
        return bitmap().size() - m_offset;
    }

    template <typename Bitmap>
    bool bitmap_offset<Bitmap>::empty() const
    {
        return size() == 0;
    }

    template <typename Bitmap>
    bitmap_offset<Bitmap>::size_type bitmap_offset<Bitmap>::null_count() const
    {
        return m_null_count;
    }

    template <typename Bitmap>
    bool bitmap_offset<Bitmap>::test(size_type pos) const
    {
        SPARROW_ASSERT_TRUE(pos < size());
        return bitmap()[pos + m_offset];
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::reference bitmap_offset<Bitmap>::at(size_type pos)
    {
        SPARROW_ASSERT_TRUE(pos < size());
        return bitmap().at(pos + m_offset);
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::const_reference bitmap_offset<Bitmap>::at(size_type pos) const
    {
        SPARROW_ASSERT_TRUE(pos < size());
        return bitmap().at(pos + m_offset);
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::reference bitmap_offset<Bitmap>::operator[](size_type i)
    {
        return at(i);
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::const_reference bitmap_offset<Bitmap>::operator[](size_type i) const
    {
        return at(i);
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::iterator bitmap_offset<Bitmap>::begin()
    {
        return sparrow::next(bitmap().begin(), m_offset);
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::const_iterator bitmap_offset<Bitmap>::begin() const
    {
        return sparrow::next(bitmap().cbegin(), m_offset);
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::iterator bitmap_offset<Bitmap>::end()
    {
        return sparrow::next(begin(), size());
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::const_iterator bitmap_offset<Bitmap>::end() const
    {
        return sparrow::next(begin(), size());
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::const_iterator bitmap_offset<Bitmap>::cbegin() const
    {
        return begin();
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::const_iterator bitmap_offset<Bitmap>::cend() const
    {
        return end();
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::reference bitmap_offset<Bitmap>::front()
    {
        SPARROW_ASSERT_TRUE(!empty());
        return at(0);
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::const_reference bitmap_offset<Bitmap>::front() const
    {
        SPARROW_ASSERT_TRUE(!empty());
        return at(0);
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::reference bitmap_offset<Bitmap>::back()
    {
        SPARROW_ASSERT_TRUE(!empty());
        return at(size() - 1);
    }

    template <typename Bitmap>
    typename bitmap_offset<Bitmap>::const_reference bitmap_offset<Bitmap>::back() const
    {
        SPARROW_ASSERT_TRUE(!empty());
        return at(size() - 1);
    }
}
