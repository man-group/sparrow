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

#include <sparrow/array/data_traits.hpp>

/////////////////////////////////////////////////////////////////////////////////////////
// Opt-in support for custom C++ representations of arrow data types.

struct MyDataType
{
};

template <>
struct sparrow::arrow_traits<MyDataType>
{
    static constexpr data_type type_id = sparrow::data_type::INT32;
    using value_type = MyDataType;
    using default_layout = sparrow::fixed_size_layout<MyDataType>;
};

static_assert(sparrow::is_arrow_traits<sparrow::arrow_traits<MyDataType>>);
static_assert(sparrow::any_arrow_type<MyDataType>);

//////////////////////////////////////////////////////////////////////////////////////////
// Base arrow types representations support tests and concept checking.
namespace sparrow
{
    static_assert(mpl::all_of(all_base_types_t{}, predicate::is_arrow_base_type));
    static_assert(mpl::all_of(all_base_types_t{}, predicate::has_arrow_traits));
}
