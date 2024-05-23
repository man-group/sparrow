#include "array_data_creation.hpp"

#include <date/date.h>
#include <date/tz.h>

namespace sparrow::test
{
    using sys_time = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;

    using namespace date::literals;

    template <>
    sparrow::array_data
    make_test_array_data<sparrow::timestamp>(size_t n, size_t offset, const std::vector<size_t>& false_bitmap)
    {
        sparrow::array_data ad;
        ad.type = sparrow::data_descriptor(sparrow::arrow_traits<sparrow::timestamp>::type_id);
        ad.bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);
        for (const auto i : false_bitmap) {
            if (i >= n) {
                throw std::invalid_argument("Index out of range");
            }
            ad.bitmap.set(i, false);
        }
        const size_t buffer_size = (n * sizeof(sparrow::timestamp)) / sizeof(uint8_t);
        sparrow::buffer<uint8_t> b(buffer_size);

        for (uint8_t i = 0; i < n; ++i)
        {
            b.data<sparrow::timestamp>()[i] = sparrow::timestamp(date::sys_days(1970_y/date::January/1_d) + date::days(i));
        }

        ad.buffers.push_back(b);
        ad.length = n;
        ad.offset = offset;
        ad.child_data.emplace_back();
        return ad;
    }
}  // namespace sparrow::test
