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


#pragma once

#include <map>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <sparrow/array.hpp>
#include <sparrow/builder/builder_utils.hpp>
#include <sparrow/builder/nested_eq.hpp>
#include <sparrow/builder/nested_less.hpp>
#include <sparrow/layout/dictionary_encoded_array.hpp>
#include <sparrow/layout/fixed_width_binary_array.hpp>
#include <sparrow/layout/list_layout/list_array.hpp>
#include <sparrow/layout/primitive_array.hpp>
#include <sparrow/layout/struct_layout/struct_array.hpp>
#include <sparrow/layout/temporal/date_array.hpp>
#include <sparrow/layout/union_array.hpp>
#include <sparrow/layout/variable_size_binary_layout/variable_size_binary_array.hpp>
#include <sparrow/utils/ranges.hpp>

#include "sparrow/layout/temporal/interval_array.hpp"

namespace sparrow
{

    // forward declaration
    namespace detail
    {
        template <class T, class LAYOUT_POLICY, class OPTIONS_TYPE>
        struct builder;
    }

    struct dense_union_flag_t
    {
    };

    struct sparse_union_flag_t
    {
    };

    struct large_list_flag_t
    {
    };

    struct large_binary_flag_t
    {
    };

    // option flag to indicate the desire for large lists
    inline constexpr large_list_flag_t large_list_flag;

    /**
     * @brief  function to create a sparrow array from arbitrary  nested
     * combinations of ranges, tuples, and nullable types, variants.
     * Have a look at the  \ref builder "buider documentation" for more information.
     */
    template <class T, class... OPTION_FLAGS>
    auto build(T&& t, OPTION_FLAGS&&...)
    {
        // for toplevel build calls, the layout policy is determined by the type itself
        using decayed_t = std::decay_t<T>;
        using layout_policy = detail::layout_flag_t<decayed_t>;
        using option_flags_type = sparrow::mpl::typelist<std::decay_t<OPTION_FLAGS>...>;

        if constexpr (detail::is_express_layout_desire<decayed_t>)
        {
            // directely unpack
            using value_type = typename decayed_t::value_type;
            return detail::builder<value_type, layout_policy, option_flags_type>::create(
                std::forward<T>(t).get()
            );
        }
        else if constexpr (detail::is_nullable_like<T>)
        {
            static_assert(mpl::dependent_false<T>::value, "toplevel type must not be nullable");
        }
        else
        {
            return detail::builder<T, layout_policy, option_flags_type>::create(std::forward<T>(t));
        }
    }

    template <class T, class... OPTION_FLAGS>
    auto build(std::initializer_list<T> t, OPTION_FLAGS&&... flags)
    {
        auto subranges = std::views::all(t);
        return build(std::forward<decltype(subranges)>(subranges), std::forward<OPTION_FLAGS>(flags)...);
    }

    namespace detail
    {

        // this is called by the nested recursive calls
        template <class LAYOUT_POLICY, class T, class... OPTION_FLAGS>
        auto build_impl(T&& t, [[maybe_unused]] sparrow::mpl::typelist<OPTION_FLAGS...> typelist)
        {
            using option_flags_type = sparrow::mpl::typelist<OPTION_FLAGS...>;
            return builder<T, LAYOUT_POLICY, option_flags_type>::create(std::forward<T>(t));
        }

        template <class T>
        concept translates_to_primitive_layout = std::ranges::input_range<T>
                                                 && std::is_scalar_v<ensured_range_value_t<T>>;

        template <typename T>
        concept translates_to_date_layout = std::ranges::input_range<T>
                                            && mpl::any_of(
                                                date_types_t{},
                                                mpl::predicate::same_as<ensured_range_value_t<T>>{}
                                            );
        template <typename T>
        concept translates_to_duration_layout = std::ranges::input_range<T>
                                                && mpl::any_of(
                                                    duration_types_t{},
                                                    mpl::predicate::same_as<ensured_range_value_t<T>>{}
                                                );
        template <typename T>
        concept translates_to_timestamp_layout = std::ranges::input_range<T>
                                                 && mpl::is_type_instance_of_v<ensured_range_value_t<T>, timestamp>;


        template <typename T>
        concept translates_to_interval_layout = std::ranges::input_range<T>
                                                && mpl::any_of(
                                                    interval_types_t{},
                                                    mpl::predicate::same_as<ensured_range_value_t<T>>{}
                                                );

        template <class T>
        concept translate_to_variable_sized_list_layout = std::ranges::input_range<T>
                                                          && std::ranges::input_range<ensured_range_value_t<T>>
                                                          && !tuple_like<ensured_range_value_t<T>>
                                                          &&  // tuples go to struct layout
                                                          // value type of inner should not be 'char-like'(
                                                          // char, byte, uint8), these are handled by
                                                          // variable_size_binary_array
                                                          !mpl::char_like<nested_ensured_range_inner_value_t<T>>;

        template <class T>
        concept translate_to_struct_layout = std::ranges::input_range<T> && tuple_like<ensured_range_value_t<T>>
                                             && !all_elements_same<ensured_range_value_t<T>>;

        template <class T>
        concept fixed_width_binary_types = (mpl::fixed_size_span<T> || mpl::std_array<T>)
                                           && std::is_same_v<std::ranges::range_value_t<T>, byte_t>;

        template <class T>
        concept translate_to_fixed_sized_list_layout = std::ranges::input_range<T>
                                                       && tuple_like<ensured_range_value_t<T>>
                                                       && !((mpl::fixed_size_span<ensured_range_value_t<T>> || mpl::std_array<ensured_range_value_t<T>>) && fixed_width_binary_types<ensured_range_value_t<T>>)
                                                       && all_elements_same<ensured_range_value_t<T>>;

        template <class T>
        concept translate_to_variable_sized_binary_layout = std::ranges::input_range<T>
                                                            && std::ranges::input_range<ensured_range_value_t<T>>
                                                            && !((mpl::fixed_size_span<ensured_range_value_t<T>> || mpl::std_array<ensured_range_value_t<T>>) && fixed_width_binary_types<ensured_range_value_t<T>>)
                                                            && !tuple_like<ensured_range_value_t<T>>
                                                            &&  // tuples go to struct layout
                                                            // value type of inner must be char like ( char,
                                                            // byte, uint8)
                                                            mpl::char_like<nested_ensured_range_inner_value_t<T>>;

        template <class T>
        concept translate_to_fixed_width_binary_layout = std::ranges::input_range<T>
                                                         && ((mpl::fixed_size_span<ensured_range_value_t<T>>
                                                              || mpl::std_array<ensured_range_value_t<T>>)
                                                             && fixed_width_binary_types<ensured_range_value_t<T>>);

        template <class T>
        concept translate_to_union_layout = std::ranges::input_range<T> &&
                                            // value type must be a variant-like type
                                            // *NOTE* we don't check for nullable here, as we want to handle
                                            // nullable variants as in the arrow spec, the nulls are handled
                                            // by the elements **in** the variant
                                            variant_like<std::ranges::range_value_t<T>>;

        template <translates_to_primitive_layout T, class OPTION_FLAGS>
        struct builder<T, dont_enforce_layout, OPTION_FLAGS>
        {
            using type = sparrow::primitive_array<ensured_range_value_t<T>>;

            template <class U>
            static type create(U&& t)
            {
                return type(std::forward<U>(t));
            }
        };

        template <translates_to_date_layout T, class OPTION_FLAGS>
        struct builder<T, dont_enforce_layout, OPTION_FLAGS>
        {
            using type = sparrow::date_array<ensured_range_value_t<T>>;

            template <class U>
            static type create(U&& t)
            {
                return type(std::forward<U>(t));
            }
        };

        template <translates_to_duration_layout T, class OPTION_FLAGS>
        struct builder<T, dont_enforce_layout, OPTION_FLAGS>
        {
            using type = sparrow::duration_array<ensured_range_value_t<T>>;

            template <class U>
            static type create(U&& t)
            {
                return type(std::forward<U>(t));
            }
        };

        template <translates_to_timestamp_layout T, class OPTION_FLAGS>
        struct builder<T, dont_enforce_layout, OPTION_FLAGS>
        {
            using type = sparrow::timestamp_array<ensured_range_value_t<T>>;
            using timezone_ptr = std::decay_t<decltype(std::declval<ensured_range_value_t<T>>().get_time_zone())>;

            template <class U>
            static type create(U&& t)
            {
                timezone_ptr tz = [&t]() -> timezone_ptr
                {
                    if (t.empty())
                    {
                        return nullptr;
                    }
                    else
                    {
                        return t.begin()->get_time_zone();
                    }
                }();
                return type(tz, std::forward<U>(t));
            }
        };

        template <translates_to_interval_layout T, class OPTION_FLAGS>
        struct builder<T, dont_enforce_layout, OPTION_FLAGS>
        {
            using type = sparrow::interval_array<ensured_range_value_t<T>>;

            template <class U>
            static type create(U&& t)
            {
                return type(std::forward<U>(t));
            }
        };

        template <translate_to_variable_sized_list_layout T, class OPTION_FLAGS>
        struct builder<T, dont_enforce_layout, OPTION_FLAGS>
        {
            using raw_value_type = std::ranges::range_value_t<T>;

            using type = std::conditional_t<
                mpl::contains<large_list_flag_t>(OPTION_FLAGS{}),
                sparrow::big_list_array,
                sparrow::list_array>;

            template <class U>
            static type create(U&& t)
            {
                auto flat_list_view = std::ranges::views::join(ensure_value_range(t));

                auto sizes = t
                             | std::views::transform(
                                 [](const auto& l)
                                 {
                                     return get_size_save(l);
                                 }
                             );

                // when the raw_value_type is a "express layout desire" we need to
                // propagate this information to the builder, so it can handle the
                using layout_policy_type = layout_flag_t<raw_value_type>;
                auto typed_array = build_impl<layout_policy_type>(flat_list_view, OPTION_FLAGS{});
                auto detyped_array = array(std::move(typed_array));

                return type(std::move(detyped_array), type::offset_from_sizes(sizes), where_null(t));
            }
        };

        template <translate_to_fixed_sized_list_layout T, class OPTION_FLAGS>
        struct builder<T, dont_enforce_layout, OPTION_FLAGS>
        {
            using type = sparrow::fixed_sized_list_array;
            static constexpr std::size_t
                list_size = std::tuple_size_v<look_trough_t<std::ranges::range_value_t<T>>>;
            using raw_value_type = std::ranges::range_value_t<T>;

            template <class U>
            static type create(U&& t)
            {
                auto flat_list_view = std::ranges::views::join(ensure_value_range(t));

                // when the raw_value_type is a "express layout desire" we need to
                // propagate this information to the builder.
                using layout_policy_type = layout_flag_t<raw_value_type>;

                return type(
                    static_cast<std::uint64_t>(list_size),
                    array(build_impl<layout_policy_type>(flat_list_view, OPTION_FLAGS{})),
                    where_null(t)
                );
            }
        };

        template <translate_to_struct_layout T, class OPTION_FLAGS>
        struct builder<T, dont_enforce_layout, OPTION_FLAGS>
        {
            using type = sparrow::struct_array;
            static constexpr std::size_t n_children = std::tuple_size_v<mnv_t<std::ranges::range_value_t<T>>>;
            using tuple_type = ensured_range_value_t<T>;

            template <class U>
            static type create(U&& t)
            {
                std::vector<array> detyped_children(n_children);
                for_each_index<n_children>(
                    [&](auto i)
                    {
                        auto tuple_i_col = t
                                           | std::views::transform(
                                               [](const auto& maybe_nullable_tuple)
                                               {
                                                   const auto& tuple_val = ensure_value(maybe_nullable_tuple);
                                                   return std::get<decltype(i)::value>(tuple_val);
                                               }
                                           );

                        using tuple_element_type = std::tuple_element_t<decltype(i)::value, tuple_type>;
                        using layout_policy_type = layout_flag_t<tuple_element_type>;
                        detyped_children[decltype(i)::value] = array(
                            build_impl<layout_policy_type>(tuple_i_col, OPTION_FLAGS{})
                        );
                    }
                );

                return type(std::move(detyped_children), where_null(t));
            }
        };

        template <translate_to_variable_sized_binary_layout T, class OPTION_FLAGS>
        struct builder<T, dont_enforce_layout, OPTION_FLAGS>
        {
            using type = sparrow::string_array;

            template <class U>
            static type create(U&& t)
            {
                auto flat_list_view = std::ranges::views::join(ensure_value_range(t));
                u8_buffer<char> data_buffer(flat_list_view);

                auto sizes = t
                             | std::views::transform(
                                 [](const auto& l)
                                 {
                                     return get_size_save(l);
                                 }
                             );

                return type(std::move(data_buffer), type::offset_from_sizes(sizes), where_null(t));
            }
        };

        template <translate_to_fixed_width_binary_layout T, class OPTION_FLAGS>
        struct builder<T, dont_enforce_layout, OPTION_FLAGS>
        {
            using type = sparrow::fixed_width_binary_array;

            template <class U>
            static type create(U&& t)
            {
                return type(std::move(t));
            }
        };

        template <translate_to_union_layout T, class OPTION_FLAGS>
        struct builder<T, dont_enforce_layout, OPTION_FLAGS>
        {
            using type = sparrow::sparse_union_array;  // TODO use options to select between sparse and dense
            using variant_type = std::ranges::range_value_t<T>;
            static constexpr std::size_t variant_size = std::variant_size_v<variant_type>;

            template <class U>
            static type create(U&& t)
                requires(std::is_same_v<type, sparrow::sparse_union_array>)
            {
                std::vector<array> detyped_children(variant_size);
                for_each_index<variant_size>(
                    [&](auto i)
                    {
                        using type_at_index = std::variant_alternative_t<decltype(i)::value, variant_type>;
                        auto type_i_col = t
                                          | std::views::transform(
                                              [](const auto& variant)
                                              {
                                                  return variant.index() == decltype(i)::value
                                                             ? std::get<type_at_index>(variant)
                                                             : type_at_index{};
                                              }
                                          );

                        using layout_policy_type = layout_flag_t<type_at_index>;
                        detyped_children[decltype(i)::value] = array(
                            build_impl<layout_policy_type>(type_i_col, OPTION_FLAGS{})
                        );
                    }
                );

                // type-ids
                auto type_id_range = t
                                     | std::views::transform(
                                         [](const auto& v)
                                         {
                                             return static_cast<std::uint8_t>(v.index());
                                         }
                                     );
                sparrow::u8_buffer<std::uint8_t> type_id_buffer(type_id_range);

                return type(std::move(detyped_children), std::move(type_id_buffer));
            }
        };

        template <class T, class OPTION_FLAGS>
        struct builder<T, enforce_dict_encoded_layout, OPTION_FLAGS>
        {
            using key_type = std::uint64_t;
            using type = sparrow::dictionary_encoded_array<key_type>;
            // keep the nulls
            using raw_range_value_type = std::ranges::range_value_t<T>;

            template <class U>
            static type create(U&& t)
            {
                const auto input_size = range_size(t);
                key_type key = 0;
                std::map<raw_range_value_type, key_type, nested_less<raw_range_value_type>> value_map;
                std::vector<raw_range_value_type> values;
                std::vector<key_type> keys;

                values.reserve(input_size);
                keys.reserve(input_size);

                for (const auto& v : t)
                {
                    auto find_res = value_map.find(v);
                    if (find_res == value_map.end())
                    {
                        value_map.insert({v, key});
                        values.push_back(v);
                        keys.push_back(key);
                        ++key;
                    }
                    else
                    {
                        keys.push_back(find_res->second);
                    }
                }
                auto keys_buffer = sparrow::u8_buffer<key_type>(keys);

                // since we do not support dict[dict or dict[run_end
                // we can hard code the layout policy here
                using layout_policy_type = dont_enforce_layout;

                auto values_array = build_impl<layout_policy_type>(values, OPTION_FLAGS{});

                return type(std::move(keys_buffer), array(std::move(values_array)));
            }
        };

        template <class T, class OPTION_FLAGS>
        struct builder<T, enforce_run_end_encoded_layout, OPTION_FLAGS>
        {
            using type = sparrow::run_end_encoded_array;
            using raw_range_value_type = std::ranges::range_value_t<T>;

            template <class U>
            static type create(U&& t)
            {
                using value_type = std::decay_t<raw_range_value_type>;

                const auto input_size = range_size(t);

                std::vector<value_type> values{};
                std::vector<std::size_t> acc_run_lengths{};

                values.reserve(input_size);
                acc_run_lengths.reserve(input_size);

                const auto eq = nested_eq<value_type>{};

                // accumulate the run lengths
                std::size_t i = 0;
                for (const auto& v : t)
                {
                    // first value
                    if (i == 0)
                    {
                        values.push_back(v);
                    }
                    // rest
                    else
                    {
                        if (!eq(values.back(), v))
                        {
                            acc_run_lengths.push_back(i);
                            values.push_back(v);
                        }
                    }
                    ++i;
                }
                acc_run_lengths.push_back(i);

                auto run_length_typed_array = primitive_array<std::size_t>(acc_run_lengths);

                // since we do not support dict[dict or dict[run_end
                // we can hard code the layout policy here
                using layout_policy_type = dont_enforce_layout;
                auto values_array = build_impl<layout_policy_type>(values, OPTION_FLAGS{});

                return type(array(std::move(run_length_typed_array)), array(std::move(values_array)));
            }
        };
    }  // namespace detail
}  // namespace sparrow
