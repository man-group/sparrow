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

#include <utility>

#include "sparrow/arrow_array_schema_proxy.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"

#include "external_array_data_creation.hpp"

namespace test
{
    using buffer_type = sparrow::buffer<uint8_t>;
    using buffer_list = std::vector<buffer_type>;

    namespace detail
    {
        static constexpr std::size_t number_children = 4;

        inline buffer_list get_test_buffer_list0()
        {
            buffer_list res = {buffer_type({0xF3, 0xFF}), buffer_type({0, 1, 2, 3, 4, 5, 6, 7, 8, 9})};
            return res;
        }

        inline buffer_list get_test_buffer_list1()
        {
            buffer_list res = {buffer_type({0xF3}), buffer_type({9, 8, 7, 6, 5})};
            return res;
        }
    }

    inline ArrowArray make_arrow_array(bool with_children)
    {
        ArrowArray res;
        if (with_children)
        {
            auto nb_children = detail::number_children;
            auto children = new ArrowArray*[nb_children];
            for (std::size_t i = 0; i < nb_children; ++i)
            {
                children[i] = new ArrowArray(make_arrow_array(false));
            }
            auto dict = new ArrowArray(make_arrow_array(false));
            sparrow::fill_arrow_array(res, 5, 2, 0, detail::get_test_buffer_list1(), nb_children, children, dict);
        }
        else
        {
            sparrow::fill_arrow_array(res, 10, 2, 0, detail::get_test_buffer_list0(), 0, nullptr, nullptr);
        }
        return res;
    }

    inline ArrowSchema make_arrow_schema(bool with_children)
    {
        using namespace std::literals;
        ArrowSchema res;
        if (with_children)
        {
            ArrowSchema** children = new ArrowSchema*[detail::number_children];
            auto nb_children = static_cast<int64_t>(detail::number_children);
            for (size_t i = 0; i < detail::number_children; ++i)
            {
                children[i] = new ArrowSchema(make_arrow_schema(false));
            }
            auto dict = new ArrowSchema(make_arrow_schema(false));
            sparrow::fill_arrow_schema(
                res,
                "c"sv,
                "with_children"sv,
                "meta1"sv,
                std::nullopt,
                nb_children,
                children,
                dict
            );
        }
        else
        {
            sparrow::fill_arrow_schema(res, "c"sv, "no_children"sv, "meta0"sv, std::nullopt, 0, nullptr, nullptr);
        }
        return res;
    }

    inline sparrow::arrow_array_and_schema make_arrow_schema_and_array(bool with_children)
    {
        return {make_arrow_array(with_children), make_arrow_schema(with_children)};
    }
}

inline std::pair<ArrowArray, ArrowSchema> make_external_arrow_schema_and_array()
{
    std::pair<ArrowArray, ArrowSchema> pair;
    constexpr size_t size = 10;
    constexpr size_t offset = 1;
    sparrow::test::fill_external_schema_and_array<uint32_t>(pair.second, pair.first, size, offset, {2, 3});
    return pair;
}
