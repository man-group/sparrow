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

    template<class ITERATOR,  bool IS_CONST>
    //class list_value : public detail::subrange<ITERATOR>
    class list_value : public std::ranges::subrange<ITERATOR>
    {
        public:
        using base_type = std::ranges::subrange<ITERATOR>;
        using base_type::base_type;
        using reference =  typename ITERATOR::reference;
        using base_type::operator=;
        using difference_type = typename std::iterator_traits<ITERATOR>::difference_type;

        reference operator[](std::size_t index)
        {
            return this->begin()[static_cast<difference_type>(index)];
        }
        reference operator[](std::size_t index)const
        {
            return this->begin()[static_cast<difference_type>(index)];
        }

        template<class OTHER_ITERATOR, bool OTHER_IS_CONST>
        bool operator==(const list_value<OTHER_ITERATOR, OTHER_IS_CONST>& rhs) const
        {
            if(this->size() != rhs.size())
            {
                return false;
            }
            for(std::size_t i = 0; i < this->size(); ++i)
            {
                if(this->operator[](i) != rhs[i])
                {
                    return false;
                }
            }
            return true;
        }

        template<class OTHER_ITERATOR, bool OTHER_IS_CONST>
        bool operator!=(const list_value<OTHER_ITERATOR, OTHER_IS_CONST>& rhs) const
        {
            return !(*this == rhs);
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
