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

#include <iterator>

namespace sparrow
{   
    /// \cond
    namespace detail
    {
        template <typename Iter>
        class subrange {
        public:
            using iterator = Iter;
            using value_type = typename std::iterator_traits<iterator>::value_type;
            using difference_type = typename std::iterator_traits<Iter>::difference_type;
            subrange() = default;
            // default copy constructor
            subrange(const subrange&) = default;
            // default move constructor
            subrange(subrange&&) = default;
            // default copy assignment
            subrange& operator=(const subrange&) = default;
            // default move assignment
            subrange& operator=(subrange&&) = default;


            subrange(iterator begin, iterator end)
                : begin_(begin), end_(end) {}

            iterator begin() const { return begin_; }
            iterator end() const { return end_; }
            iterator cbegin() const { return begin_; }
            iterator cend() const { return end_; }

            std::size_t size() const { return static_cast<std::size_t>(std::distance(begin_, end_)); }

        private:
            iterator begin_;
            iterator end_;
        };
    }
    /// \endcond

    template<class ITERATOR,  bool IS_CONST>
    class list_value : public detail::subrange<ITERATOR>
    {
        public:
        using base_type = detail::subrange<ITERATOR>;
        using base_type::base_type;
        using reference =  typename ITERATOR::reference;
        using base_type::operator=;
        using difference_type = typename std::iterator_traits<ITERATOR>::difference_type;

        reference operator[](std::size_t index)
        {
            return this->begin()[static_cast<difference_type>(index)];
        }
    };

    template<class T>
    struct is_list_value : std::false_type
    {
    };
    template<class ITER, bool IS_CONST>
    struct is_list_value<list_value<ITER, IS_CONST>> : std::true_type
    {
    };

    template<class T>
    inline constexpr bool is_list_value_v = is_list_value<T>::value;
}
