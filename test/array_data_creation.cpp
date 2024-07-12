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
        ad.m_type = sparrow::data_descriptor(sparrow::arrow_traits<sparrow::timestamp>::type_id);
        ad.m_bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);
        for (const auto i : false_bitmap)
        {
            if (i >= n)
            {
                throw std::invalid_argument("Index out of range");
            }
            ad.m_bitmap.set(i, false);
        }
        const std::size_t buffer_size = (n * sizeof(sparrow::timestamp)) / sizeof(uint8_t);
        sparrow::buffer<uint8_t> b(buffer_size);

        for (uint8_t i = 0; i < n; ++i)
        {
            b.data<sparrow::timestamp>()[i] = sparrow::timestamp(
                date::sys_days(date::year(1970) / date::January / date::day(1)) + date::days(i)
            );
        }

        ad.m_buffers.push_back(b);
        ad.m_length = static_cast<std::int64_t>(n);
        ad.m_offset = static_cast<std::int64_t>(offset);
        ad.m_child_data.emplace_back();
        return ad;
    }
}  // namespace sparrow::test
