/**
 * @example timestamp_example.cpp
 * Example of usage of the timestamp_array class.
 */

#include <cassert>

#include "sparrow/timestamp_array.hpp"

int main()
{
    //! [use_timestamp_array]
    constexpr uint64_t epoch = 1737639900;  // Thursday January 23, 2025 08:45:00
    const date::time_zone* new_york = date::locate_zone("America/New_York");
    const date::time_zone* paris = date::locate_zone("Europe/Paris");
    std::vector<sparrow::timestamp<std::chrono::seconds>> input_values;
    input_values.emplace_back(new_york, date::sys_time<std::chrono::seconds>{std::chrono::seconds(epoch)});
    const sparrow::timestamp_array<sparrow::timestamp<std::chrono::seconds>> ar(paris, input_values);
    assert(ar.size() == 1);
    assert(ar[0].has_value());
    assert(ar[0].value().get_sys_time().time_since_epoch().count() == epoch);
    //! [use_timestamp_array]
    return EXIT_SUCCESS;
}
