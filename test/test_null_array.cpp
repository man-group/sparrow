// Man Group Operations Limited
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

#include <doctest/doctest.h>

#include <sparrow/layout/null_array.hpp>
#include <sparrow/utils/nullable.hpp>
#include <sparrow/utils/ranges.hpp>

#include "external_array_data_creation.hpp"
#include "metadata_sample.hpp"


namespace sparrow
{
    using test::make_arrow_proxy;

    TEST_SUITE("null array")
    {
        static_assert(is_null_array_v<null_array>);

        TEST_CASE("constructor")
        {
            constexpr std::size_t size = 10u;
            constexpr std::string_view name = "name";
            const null_array ar{size, name, metadata_sample_opt};
            CHECK_EQ(ar.name(), name);
            test_metadata(metadata_sample, *(ar.metadata()));
            CHECK_EQ(ar.size(), size);

            const auto& arrow_proxy = sparrow::detail::array_access::get_arrow_proxy(ar);
            CHECK_EQ(arrow_proxy.format(), "n");
            CHECK_EQ(arrow_proxy.n_children(), 0);
            CHECK(arrow_proxy.flags().empty());
            test_metadata(metadata_sample, *(arrow_proxy.metadata()));
            CHECK_EQ(arrow_proxy.name(), name);
            CHECK_EQ(arrow_proxy.dictionary(), nullptr);

            CHECK_EQ(arrow_proxy.buffers().size(), 0);
        }

        TEST_CASE("copy")
        {
            constexpr std::size_t size = 10u;
            const null_array ar{size};
            const null_array ar2(ar);
            CHECK_EQ(ar, ar2);

            null_array ar3{size + 2u};
            CHECK_NE(ar, ar3);
            ar3 = ar;
            CHECK_EQ(ar, ar3);
        }

        TEST_CASE("move")
        {
            constexpr std::size_t size = 10u;
            null_array ar{size};
            null_array ar2(ar);
            null_array ar3(std::move(ar));
            CHECK_EQ(ar3, ar2);

            null_array ar4{size + 3u};
            CHECK_NE(ar4, ar2);
            ar4 = std::move(ar3);
            CHECK_EQ(ar2, ar4);
        }

        TEST_CASE("operator[]")
        {
            constexpr std::size_t size = 10u;
            null_array ar{size};
            const null_array car{size};

            CHECK_EQ(ar[2], nullval);
            CHECK_EQ(car[2], nullval);
        }

        TEST_CASE("iterator")
        {
            constexpr std::size_t size = 3u;
            null_array ar{size};

            auto iter = ar.begin();
            auto citer = ar.cbegin();
            CHECK_EQ(*iter, nullval);
            CHECK_EQ(*citer, nullval);

            ++iter;
            ++citer;
            CHECK_EQ(*iter, nullval);
            CHECK_EQ(*citer, nullval);

            iter += 2;
            citer += 2;
            CHECK_EQ(iter, ar.end());
            CHECK_EQ(citer, ar.cend());
        }

        TEST_CASE("const_value_iterator")
        {
            constexpr std::size_t size = 3u;
            null_array ar{size};

            auto value_range = ar.values();
            auto iter = value_range.begin();
            CHECK_EQ(*iter, 0);
            iter += 3;
            CHECK_EQ(iter, value_range.end());
        }

        TEST_CASE("const_bitmap_iterator")
        {
            constexpr std::size_t size = 3u;
            null_array ar{size};

            auto bitmap_range = ar.bitmap();
            auto iter = bitmap_range.begin();
            CHECK_EQ(*iter, false);
            iter += 3;
            CHECK_EQ(iter, bitmap_range.end());
        }

#if defined(__cpp_lib_format)
        TEST_CASE("formatter")
        {
            constexpr std::size_t size = 3u;
            null_array ar(make_arrow_proxy<null_type>(size));
            const std::string expected = "Null array [3]";
            CHECK_EQ(std::format("{}", ar), expected);
        }
#endif
    }
}
