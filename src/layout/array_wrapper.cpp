#include "sparrow/layout/array_wrapper.hpp"

#include "sparrow/layout/array_registry.hpp"

#include <type_traits>

namespace sparrow
{
    void array_wrapper::insert_elements_from(
        size_type pos,
        const array_wrapper& source,
        size_type src_begin,
        size_type src_end,
        size_type count
    )
    {
        SPARROW_ASSERT_TRUE(source.data_type() == this->data_type());
        SPARROW_ASSERT_TRUE(pos <= this->get_arrow_proxy().length());
        SPARROW_ASSERT_TRUE(src_begin <= src_end);
        SPARROW_ASSERT_TRUE(src_end <= source.get_arrow_proxy().length());

        if (count == 0 || src_begin == src_end)
        {
            return;
        }

        if (typeid(*this) != typeid(source))
        {
            throw std::invalid_argument(
                "array_wrapper::insert_elements_from: source and destination must wrap the same concrete array "
                "type"
            );
        }

        visit(
            [this, &source, pos, src_begin, src_end, count](const auto& array)
            {
                using array_type = std::remove_cvref_t<decltype(array)>;
                array_type& destination = unwrap_array<array_type>(*this);
                const array_type& source_array = unwrap_array<array_type>(source);

                if constexpr (
                    requires
                    {
                        destination.insert(destination.cbegin(), source_array.cbegin(), source_array.cbegin());
                    }
                )
                {
                    const std::ptrdiff_t elem_count = static_cast<std::ptrdiff_t>(src_end - src_begin);
                    const auto source_first = std::next(source_array.cbegin(), static_cast<std::ptrdiff_t>(src_begin));
                    const auto source_last = std::next(source_array.cbegin(), static_cast<std::ptrdiff_t>(src_end));
                    using source_input_value = std::remove_cvref_t<decltype(nullable_get(*source_first))>;
                    using destination_input_value = std::remove_cvref_t<
                        decltype(nullable_get(std::declval<typename array_type::value_type&>()))>;
                    constexpr bool can_insert_directly = std::same_as<
                        source_input_value,
                        destination_input_value>;

                    if (&destination == &source_array)
                    {
                        // Materialize the slice when source and destination alias.
                        std::vector<typename array_type::value_type> temp_slice(source_first, source_last);

                        for (std::size_t i = 0; i < count; ++i)
                        {
                            auto cur_pos = std::next(
                                destination.cbegin(),
                                static_cast<std::ptrdiff_t>(pos) + static_cast<std::ptrdiff_t>(i) * elem_count
                            );
                            destination.insert(cur_pos, temp_slice.cbegin(), temp_slice.cend());
                        }
                    }
                    else if constexpr (can_insert_directly)
                    {
                        for (std::size_t i = 0; i < count; ++i)
                        {
                            auto cur_pos = std::next(
                                destination.cbegin(),
                                static_cast<std::ptrdiff_t>(pos) + static_cast<std::ptrdiff_t>(i) * elem_count
                            );
                            destination.insert(cur_pos, source_first, source_last);
                        }
                    }
                    else
                    {
                        // Some arrays expose view-like iterator values that cannot be reinserted directly.
                        std::vector<typename array_type::value_type> temp_slice(source_first, source_last);

                        for (std::size_t i = 0; i < count; ++i)
                        {
                            auto cur_pos = std::next(
                                destination.cbegin(),
                                static_cast<std::ptrdiff_t>(pos) + static_cast<std::ptrdiff_t>(i) * elem_count
                            );
                            destination.insert(cur_pos, temp_slice.cbegin(), temp_slice.cend());
                        }
                    }
                }
                else
                {
                    throw std::runtime_error(
                        "array_wrapper::insert_elements_from: array type does not support mutation"
                    );
                }
            },
            static_cast<const array_wrapper&>(*this)
        );
    }

    void array_wrapper::erase_array_elements(size_type pos, size_type count)
    {
        const auto length = this->get_arrow_proxy().length();
        SPARROW_ASSERT_TRUE(pos <= length);
        SPARROW_ASSERT_TRUE(count <= length - pos);

        if (count == 0)
        {
            return;
        }

        visit(
            [this, pos, count](const auto& array)
            {
                using array_type = std::remove_cvref_t<decltype(array)>;
                array_type& wrapped_array = unwrap_array<array_type>(*this);

                if constexpr (requires { wrapped_array.erase(wrapped_array.cbegin(), wrapped_array.cbegin()); })
                {
                    auto first = std::next(wrapped_array.cbegin(), static_cast<std::ptrdiff_t>(pos));
                    auto last = std::next(first, static_cast<std::ptrdiff_t>(count));
                    static_cast<void>(wrapped_array.erase(first, last));
                }
                else
                {
                    throw std::runtime_error(
                        "array_wrapper::erase_array_elements: array type does not support mutation"
                    );
                }
            },
            static_cast<const array_wrapper&>(*this)
        );
    }
}