#include "sparrow/layout/run_end_encoded_layout/run_end_encoded_array.hpp"
#include "sparrow/layout/array_helper.hpp"
#include "sparrow/layout/dispatch.hpp"
#include "sparrow/layout/primitive_array.hpp"

namespace sparrow   
{    
    template<class T>
    concept usable_array =
             mpl::is_type_instance_of_v<T, primitive_array> && (
             std::same_as<typename T::inner_value_type, std::uint16_t> ||
             std::same_as<typename T::inner_value_type, std::uint32_t> ||
             std::same_as<typename T::inner_value_type, std::uint64_t>);
    
    auto run_end_encoded_array::get_acc_lengths_ptr(const array_wrapper& ar) -> acc_length_ptr_variant_type
    {
        return visit(
            [](const auto& actual_arr) -> acc_length_ptr_variant_type
            {
                using array_type = std::decay_t<decltype(actual_arr)>;
                
                if constexpr(usable_array<array_type>)
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
                const auto it = std::upper_bound(
                    acc_lengths_ptr,
                    acc_lengths_ptr + this->m_encoded_length,
                    i
                );
                // std::lower_bound returns an iterator, so we need to convert it to an index
                const auto index = static_cast<std::size_t>(std::distance(acc_lengths_ptr, it));
                return array_element(*p_encoded_values_array, static_cast<std::size_t>(index));
            },
            m_acc_lengths
        );
    }
}
