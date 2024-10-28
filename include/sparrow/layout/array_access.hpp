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

namespace sparrow::detail
{
    
    class array_access
    {
    public:
        template<class ARRAY>
        static inline const auto & storage(const ARRAY& array) 
        {
            return array.storage();
        }

        template<class ARRAY>
        static inline auto & storage(ARRAY& array)
        {
            return array.storage();
        }

        template<class ARRAY>
        requires(std::is_rvalue_reference_v<ARRAY&&>)
        static inline auto && extract_arrow_proxy(ARRAY&& array)
        {
            return std::move(array).extract_arrow_proxy();
        }

        template<class ARRAY>
        requires(std::is_rvalue_reference_v<ARRAY&&>)
        static inline auto  extract_array_wrapper(ARRAY&& array)
        {
            return std::move(array).extract_array_wrapper();
        }
    };
}