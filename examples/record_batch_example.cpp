/** @example record_batch_example.cpp
 * Example of usage of the record_batch class.
 */


#include <cassert>

#include "sparrow/layout/primitive_layout/primitive_array.hpp"
#include "sparrow/record_batch.hpp"

std::vector<sparrow::array> make_array_list(const std::size_t data_size)
{
    auto iota = std::ranges::iota_view{std::size_t(0), std::size_t(data_size)};
    sparrow::primitive_array<std::uint16_t> pr0(
        iota
        | std::views::transform(
            [](auto i)
            {
                return static_cast<std::uint16_t>(i);
            }
        )
    );
    auto iota2 = std::ranges::iota_view{std::int32_t(4), 4 + std::int32_t(data_size)};
    sparrow::primitive_array<std::int32_t> pr1(iota2);
    auto iota3 = std::ranges::iota_view{std::int32_t(2), 2 + std::int32_t(data_size)};
    sparrow::primitive_array<std::int32_t> pr2(iota3);

    std::vector<sparrow::array> arr_list = {
        sparrow::array{std::move(pr0)},
        sparrow::array{std::move(pr1)},
        sparrow::array{std::move(pr2)}
    };
    return arr_list;
}

int main()
{
    //! [use_record_batch]
    const std::vector<std::string> name_list = {"first", "second", "third"};
    constexpr std::size_t data_size = 10;
    const std::vector<sparrow::array> array_list = make_array_list(data_size);
    const sparrow::record_batch record{name_list, array_list, "record batch name"};
    assert(record.name() == "record batch name");
    assert(record.nb_columns() == array_list.size());
    assert(record.nb_rows() == data_size);
    assert(record.contains_column(name_list[0]));
    assert(record.get_column_name(0) == name_list[0]);
    assert(record.get_column(0) == array_list[0]);
    assert(std::ranges::equal(record.names(), name_list));
    assert(std::ranges::equal(record.columns(), array_list));
    //! [use_record_batch]
    return EXIT_SUCCESS;
}
