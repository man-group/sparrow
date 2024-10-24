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

#include <cstddef>
#include <vector>

#include "doctest/doctest.h"

#include "sparrow/types/arrow_length.hpp"

#if defined(__GNUC__) && !defined(__clang__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wuseless-cast" // We want to be able to cast type aliases to their real types.
#endif

TEST_SUITE("value_ptr")
{
    TEST_CASE("is_valid_arrow_length")
    {
        using namespace sparrow;
        CHECK(is_valid_arrow_length(0));
        CHECK(is_valid_arrow_length(1));
        CHECK(is_valid_arrow_length(1024));
        CHECK(is_valid_arrow_length(std::numeric_limits<std::int32_t>::max()));
        CHECK(is_valid_arrow_length(max_arrow_length));

        CHECK_FALSE(is_valid_arrow_length(-1));
        CHECK_FALSE(is_valid_arrow_length(-1024));
        CHECK_FALSE(is_valid_arrow_length(std::numeric_limits<std::int32_t>::min()));
        CHECK(is_valid_arrow_length(-1, arrow_length_kind::offset));
        CHECK(is_valid_arrow_length(-1024, arrow_length_kind::offset));
        CHECK(is_valid_arrow_length(std::numeric_limits<std::int32_t>::min(), arrow_length_kind::offset));

        CHECK(is_valid_arrow_length(std::size_t{0}));
        CHECK(is_valid_arrow_length(std::size_t{1}));
        CHECK(is_valid_arrow_length(std::ptrdiff_t{0}));
        CHECK(is_valid_arrow_length(std::ptrdiff_t{1}));
        CHECK(is_valid_arrow_length(std::ptrdiff_t{-1}, arrow_length_kind::offset));

        // When not contrained to 32bit length and native offsets can represent less or equal values of arrow lengh,
        // we check that the native offset types are usable as expected as arrow length.
        if constexpr( sizeof(std::ptrdiff_t) <= max_arrow_length and config::enable_32bit_size_limit == false )
        {
            CHECK(is_valid_arrow_length(std::numeric_limits<std::ptrdiff_t>::max()));
            CHECK(is_valid_arrow_length(std::numeric_limits<std::ptrdiff_t>::max(), arrow_length_kind::offset));
            CHECK(is_valid_arrow_length(std::numeric_limits<std::ptrdiff_t>::min(), arrow_length_kind::offset));
        }

        // We always support at least 32bit lengths
        CHECK(is_valid_arrow_length(std::size_t(std::numeric_limits<std::int32_t>::max())));
        CHECK(is_valid_arrow_length(std::ptrdiff_t(std::numeric_limits<std::int32_t>::max())));
        CHECK(is_valid_arrow_length(
            std::ptrdiff_t(std::numeric_limits<std::int32_t>::max()),
            arrow_length_kind::offset
        ));
        CHECK(is_valid_arrow_length(
            std::ptrdiff_t(std::numeric_limits<std::int32_t>::min()),
            arrow_length_kind::offset
        ));
    }

    TEST_CASE("throw_if_invalid_size")
    {
        using namespace sparrow;
        throw_if_invalid_size(0);
        throw_if_invalid_size(1);
        throw_if_invalid_size(1024);
        throw_if_invalid_size(std::numeric_limits<std::int32_t>::max());
        throw_if_invalid_size(max_arrow_length);

        if (config::enable_size_limit_runtime_check)
        {
            CHECK_THROWS(throw_if_invalid_size(-1));
            CHECK_THROWS(throw_if_invalid_size(-1024));
            CHECK_THROWS(throw_if_invalid_size(std::numeric_limits<std::int32_t>::min()));
        }

        throw_if_invalid_size(-1, arrow_length_kind::offset);
        throw_if_invalid_size(-1024, arrow_length_kind::offset);
        throw_if_invalid_size(std::numeric_limits<std::int32_t>::min(), arrow_length_kind::offset);

        throw_if_invalid_size(std::size_t{0});
        throw_if_invalid_size(std::size_t{1});
        throw_if_invalid_size(std::ptrdiff_t{0});
        throw_if_invalid_size(std::ptrdiff_t{1});
        throw_if_invalid_size(std::ptrdiff_t{-1}, arrow_length_kind::offset);

        // When not contrained to 32bit length and native offsets can represent less or equal values of arrow
        // length, we check that the native offset types are usable as expected as arrow length.
        if constexpr (sizeof(std::ptrdiff_t) <= max_arrow_length and config::enable_32bit_size_limit == false)
        {
            throw_if_invalid_size(std::numeric_limits<std::ptrdiff_t>::max());
            throw_if_invalid_size(std::numeric_limits<std::ptrdiff_t>::max(), arrow_length_kind::offset);
            throw_if_invalid_size(std::numeric_limits<std::ptrdiff_t>::min(), arrow_length_kind::offset);
        }

        // We always support at least 32bit lengths
        throw_if_invalid_size(std::size_t(std::numeric_limits<std::int32_t>::max()));
        throw_if_invalid_size(std::ptrdiff_t(std::numeric_limits<std::int32_t>::max()));
        throw_if_invalid_size(
            std::ptrdiff_t(std::numeric_limits<std::int32_t>::max()),
            arrow_length_kind::offset
        );
        throw_if_invalid_size(
            std::ptrdiff_t(std::numeric_limits<std::int32_t>::min()),
            arrow_length_kind::offset
        );
    }

}

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif
