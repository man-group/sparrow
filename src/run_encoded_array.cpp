#include "sparrow/array.hpp"
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/layout/dispatch.hpp"
#include "sparrow/layout/primitive_array.hpp"
#include "sparrow/layout/run_end_encoded_layout/run_end_encoded_array.hpp"


namespace sparrow
{
    template <class T>
    concept usable_array = mpl::is_type_instance_of_v<T, primitive_array>
                           && (std::same_as<typename T::inner_value_type, std::uint16_t>
                               || std::same_as<typename T::inner_value_type, std::uint32_t>
                               || std::same_as<typename T::inner_value_type, std::uint64_t>);

    auto run_end_encoded_array::get_acc_lengths_ptr(const array_wrapper& ar) -> acc_length_ptr_variant_type
    {
        return visit(
            [](const auto& actual_arr) -> acc_length_ptr_variant_type
            {
                using array_type = std::decay_t<decltype(actual_arr)>;

                if constexpr (usable_array<array_type>)
                {
                    return actual_arr.data();
                }
                else
                {
                    throw std::invalid_argument("array type not supported");
                }
            },
            ar
        );
    }

    auto run_end_encoded_array::operator[](std::uint64_t i) const -> array_traits::const_reference
    {
        return visit(
            [i, this](const auto& acc_lengths_ptr) -> array_traits::const_reference
            {
                const auto it = std::upper_bound(acc_lengths_ptr, acc_lengths_ptr + this->m_encoded_length, i);
                // std::lower_bound returns an iterator, so we need to convert it to an index
                const auto index = static_cast<std::size_t>(std::distance(acc_lengths_ptr, it));
                return array_element(*p_encoded_values_array, static_cast<std::size_t>(index));
            },
            m_acc_lengths
        );
    }

    std::pair<std::int64_t, std::int64_t>
    run_end_encoded_array::extract_length_and_null_count(const array& acc_lengths_arr, const array& encoded_values_arr)
    {
        SPARROW_ASSERT_TRUE(acc_lengths_arr.size() == encoded_values_arr.size());

        // get the raw null count
        std::uint64_t raw_null_count = detail::array_access::get_arrow_proxy(acc_lengths_arr).null_count();
        auto raw_size = acc_lengths_arr.size();
        // visit the acc_lengths array
        std::int64_t length = 0;
        std::int64_t null_count = 0;
        acc_lengths_arr.visit(
            [&](const auto& acc_lengths_array)
            {
                if constexpr (usable_array<std::decay_t<decltype(acc_lengths_array)>>)
                {
                    auto acc_length_data = acc_lengths_array.data();
                    // get the length of the array (ie last element in the acc_lengths array)
                    length = acc_length_data[raw_size - 1];

                    if (raw_null_count == 0)
                    {
                        return;
                    }
                    for (std::size_t i = 0; i < raw_size; ++i)
                    {
                        // check if the value is null
                        if (!encoded_values_arr[i].has_value())
                        {
                            // how often is this value repeated?
                            const auto run_length = i == 0 ? acc_length_data[i]
                                                           : acc_length_data[i] - acc_length_data[i - 1];
                            null_count += run_length;
                            raw_null_count -= 1;
                            if (raw_null_count == 0)
                            {
                                return;
                            }
                        }
                    }
                }
                else
                {
                    throw std::invalid_argument("array type not supported");
                }
            }
        );
        return {null_count, length};
    };

    auto run_end_encoded_array::create_proxy(
        array&& acc_lengths,
        array&& encoded_values,
        std::optional<std::string_view> name,
        std::optional<std::string_view> metadata
    ) -> arrow_proxy
    {
        auto [null_count, length] = extract_length_and_null_count(acc_lengths, encoded_values);

        auto [acc_length_array, acc_length_schema] = extract_arrow_structures(std::move(acc_lengths));
        auto [encoded_values_array, encoded_values_schema] = extract_arrow_structures(std::move(encoded_values));

        constexpr auto n_children = 2;
        ArrowSchema** child_schemas = new ArrowSchema*[n_children];
        ArrowArray** child_arrays = new ArrowArray*[n_children];

        child_schemas[0] = new ArrowSchema(std::move(acc_length_schema));
        child_schemas[1] = new ArrowSchema(std::move(encoded_values_schema));

        child_arrays[0] = new ArrowArray(std::move(acc_length_array));
        child_arrays[1] = new ArrowArray(std::move(encoded_values_array));

        ArrowSchema schema = make_arrow_schema(
            std::string("+r"),
            name,  // name
            metadata,  // metadata
            std::nullopt,  // flags,
            n_children,
            child_schemas,  // children
            nullptr         // dictionary
        );

        std::vector<buffer<std::uint8_t>> arr_buffs = {};

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(length),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            n_children,    // n_children
            child_arrays,  // children
            nullptr        // dictionary
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }
}
