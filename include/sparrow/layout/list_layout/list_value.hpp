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

#include <functional>
#include <ranges>
#include <string_view>

#include "sparrow/array/array_data_concepts.hpp"
#include "sparrow/array/array_data.hpp"
#include "sparrow/layout/layout_iterator.hpp"
#include "sparrow/utils/algorithm.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/iterator.hpp"
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow
{   
    /// \cond
    namespace detail
    {

        // using std::ranges::subrange is not working :/
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

            difference_type size() const { return std::distance(begin_, end_); }

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
        // using assignment operator from base class
        using base_type::operator=;

        using difference_type = typename std::iterator_traits<ITERATOR>::difference_type;


        reference operator[](std::size_t index)
        {
            return this->begin()[static_cast<difference_type>(index)];
        }

        reference at(std::size_t index) const
        {
           return this->begin()[static_cast<difference_type>(index)];
        }
        

    };
}
