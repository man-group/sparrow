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

#include "array_data_creation.hpp"

#include <chrono>
#include <stdexcept>
#include <vector>

#if defined(SPARROW_USE_DATE_POLYFILL)
#    include <date/date.h>
#    include <date/tz.h>
#else
namespace date = std::chrono;
#endif

namespace sparrow::test
{
    using sys_time = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;

    template <>
    sparrow::array_data make_test_array_data<sparrow::timestamp>(
        std::size_t n,
        std::size_t offset,
        const std::vector<std::size_t>& false_bitmap
    )
    {
        sparrow::array_data ad;
        ad.type = sparrow::data_descriptor(sparrow::arrow_traits<sparrow::timestamp>::type_id);
        ad.bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);
        for (const auto i : false_bitmap)
        {
            if (i >= n)
            {
                throw std::invalid_argument("Index out of range");
            }
            ad.bitmap.set(i, false);
        }
        const std::size_t buffer_size = (n * sizeof(sparrow::timestamp)) / sizeof(uint8_t);
        sparrow::buffer<uint8_t> b(buffer_size);

        for (uint8_t i = 0; i < n; ++i)
        {
            b.data<sparrow::timestamp>()[i] = sparrow::timestamp(
                date::sys_days(date::year(1970) / date::January / date::day(1)) + date::days(i)
            );
        }

        ad.buffers.push_back(b);
        ad.length = static_cast<std::int64_t>(n);
        ad.offset = static_cast<std::int64_t>(offset);
        ad.child_data.emplace_back();
        return ad;
    }
}  // namespace sparrow::test
