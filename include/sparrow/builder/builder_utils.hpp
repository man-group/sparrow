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

#include <array>
#include <ranges>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <sparrow/array.hpp>
#include <sparrow/utils/ranges.hpp>

namespace sparrow
{

    namespace detail
    {
        template <class T>
        class express_layout_desire
        {
        public:

            using self_type = express_layout_desire<T>;
            using value_type = T;

            // default constructor
            constexpr express_layout_desire() = default;

            // variadic perfect forwarding constructor
            template <class... Args>
            express_layout_desire(Args&&... args)
                : m_value(std::forward<Args>(args)...)
            {
            }

            [[nodiscard]] constexpr const T& get() const noexcept
            {
                return m_value;
            }

            [[nodiscard]] constexpr T& get() noexcept
            {
                return m_value;
            }

        private:

            T m_value = T{};
        };
    }

    // express the desire to use a dictionary encoding layout for
    // whatever is inside. Note that what is inside **is not yet**
    // encoded. This is done once the complete data which is to be
    // dict encoded is known
    template <class T, class KEY_TYPE = std::uint64_t>
    class dict_encode : public detail::express_layout_desire<T>
    {
        using base_type = detail::express_layout_desire<T>;

    public:

        using base_type::base_type;
        using key_type = KEY_TYPE;
    };

    // express the desire to use a run-length encoding layout for
    // whatever is inside. Note that what is inside **is not yet**
    // encoded. This is done once the complete data which is to be
    // dict encoded is known
    template <class T, class LENGTH_TYPE = std::uint64_t>
    class run_end_encode : public detail::express_layout_desire<T>
    {
        using base_type = detail::express_layout_desire<T>;

    public:

        using base_type::base_type;
        using length_type = LENGTH_TYPE;
    };

    namespace detail
    {
        // might be changed later to also allowm for std::optional
        template <class T>
        concept is_nullable_like = sparrow::is_nullable_v<T>;

        struct dont_enforce_layout
        {
        };

        struct enforce_dict_encoded_layout
        {
        };

        struct enforce_run_end_encoded_layout
        {
        };

        template <class T>
        concept is_dict_encode = sparrow::mpl::is_type_instance_of_v<T, dict_encode>;

        template <class T>
        concept is_run_end_encode = sparrow::mpl::is_type_instance_of_v<T, run_end_encode>;

        template <class T>
        concept is_express_layout_desire = is_dict_encode<T> || is_run_end_encode<T>;

        template <class T>
        concept is_plain_value_type = !is_express_layout_desire<T> && !is_nullable_like<T>;

        template <class T>
        using decayed_range_value_t = std::decay_t<std::ranges::range_value_t<T>>;

        template <class F, std::size_t... Is>
        void for_each_index_impl(F&& f, std::index_sequence<Is...>)
        {
            // Apply f to each index individually
            (f(std::integral_constant<std::size_t, Is>{}), ...);
        }

        template <std::size_t SIZE, class F>
        void for_each_index(F&& f)
        {
            for_each_index_impl(std::forward<F>(f), std::make_index_sequence<SIZE>());
        }

        template <class F, std::size_t... Is>
        bool exitable_for_each_index_impl(F&& f, std::index_sequence<Is...>)
        {
            return (f(std::integral_constant<std::size_t, Is>{}) && ...);
        }

        template <std::size_t SIZE, class F>
        bool exitable_for_each_index(F&& f)
        {
            return exitable_for_each_index_impl(std::forward<F>(f), std::make_index_sequence<SIZE>());
        }

        template <class T, std::size_t N>
        concept has_tuple_element = requires(T t) {
            typename std::tuple_element_t<N, std::remove_const_t<T>>;
            { get<N>(t) } -> std::convertible_to<const std::tuple_element_t<N, T>&>;
        };

        template <typename T, std::size_t... N>
        constexpr bool check_tuple_elements(std::index_sequence<N...>)
        {
            return (has_tuple_element<T, N> && ...);
        }

        template <typename T>
        constexpr bool is_tuple_like()
        {
            return check_tuple_elements<T>(std::make_index_sequence<std::tuple_size_v<T>>());
        }

        template <typename T>
        concept variant_like =
            // Must not be a reference
            !std::is_reference_v<T> &&

            // Must have an index() member function returning a size_t
            requires(const T& v) {
                { v.index() } -> std::convertible_to<std::size_t>;
            } &&

            // Must work with std::visit
            requires(T v) {
                std::visit([](auto&&) {}, v);  // Use a generic lambda to test visitation
            } &&

            // Must allow std::get with an index
            requires(T v) {
                { std::get<0>(v) };  // Access by index
            } &&

            // Must allow std::get with a type
            requires(T v) {
                { std::get<typename std::variant_alternative<0, T>::type>(v) };  // Access by type
            };

        template <typename T>
        concept tuple_like = !std::is_reference_v<T> && requires(T t) {
            typename std::tuple_size<T>::type;
            requires std::derived_from<std::tuple_size<T>, std::integral_constant<std::size_t, std::tuple_size_v<T>>>;
        } && is_tuple_like<T>();

        template <typename Tuple, size_t... Is>
        [[nodiscard]] constexpr bool all_elements_same_impl(std::index_sequence<Is...>)
        {
            return sizeof...(Is) == 0
                   || ((std::is_same_v<std::tuple_element_t<0, Tuple>, std::tuple_element_t<Is, Tuple>>) && ...);
        }

        template <typename T>
        concept all_elements_same = tuple_like<T>
                                    && all_elements_same_impl<T>(
                                        std::make_index_sequence<std::tuple_size_v<T>>{}
                                    );

        template <class T>
        struct maybe_nullable_value_type
        {
            using type = T;
        };

        template <is_nullable_like T>
        struct maybe_nullable_value_type<T>
        {
            using type = typename T::value_type;
        };

        // shorthand for maybe_nullable_value_type<T>::type
        template <class T>
        using mnv_t = typename maybe_nullable_value_type<T>::type;

        template <class T>
        struct maybe_express_layout_desire_value_type
        {
            using type = T;
        };

        template <is_express_layout_desire T>
        struct maybe_express_layout_desire_value_type<T>
        {
            using type = typename T::value_type;
        };
        // shorthand for maybe_express_layout_desire_value_type<T>::type
        template <class T>
        using meldv_t = typename maybe_express_layout_desire_value_type<T>::type;


        template <class T>
        using layout_flag_t = std::conditional_t<
            is_dict_encode<mnv_t<T>>,
            enforce_dict_encoded_layout,
            std::conditional_t<is_run_end_encode<mnv_t<T>>, enforce_run_end_encoded_layout, dont_enforce_layout>>;

        template <class T>
        using look_trough_t = meldv_t<mnv_t<meldv_t<T>>>;

        // shorhand for look_trough_t<std::ranges::range_value_t<T>>
        template <class T>
        using ensured_range_value_t = look_trough_t<std::ranges::range_value_t<T>>;

        // helper to get inner value type of smth like a vector of vector of T
        // we also translate any nullable to the inner type
        template <class T>
        using nested_ensured_range_inner_value_t = ensured_range_value_t<ensured_range_value_t<T>>;

        // a save way to return .size from
        // a possibly nullable object or "express layout desire object"
        template <class T>
        [[nodiscard]] constexpr std::size_t get_size_save(T&& t)
        {
            using decayed = std::decay_t<T>;
            if constexpr (is_nullable_like<decayed>)
            {
                if (t.has_value())
                {
                    return get_size_save(t.get());
                }
                else
                {
                    return 0;
                }
            }
            else if constexpr (is_express_layout_desire<decayed>)
            {
                return get_size_save(t.get());
            }
            else
            {
                return static_cast<std::size_t>(t.size());
            }
        }

        template <class T>
        [[nodiscard]] constexpr decltype(auto) ensure_value(T&& t)
        {
            using decayed = std::decay_t<T>;
            if constexpr (is_nullable_like<decayed> || is_express_layout_desire<decayed>)
            {
                return ensure_value(std::forward<T>(t).get());
            }
            else
            {
                return std::forward<T>(t);
            }
        }

        template <std::ranges::range T>
            requires(is_nullable_like<std::ranges::range_value_t<T>>)
        [[nodiscard]] constexpr std::vector<std::size_t> where_null(T&& t)
        {
            std::vector<std::size_t> result;
            for (std::size_t i = 0; i < t.size(); ++i)
            {
                if (!t[i].has_value())
                {
                    result.push_back(i);
                }
            }
            return result;
        }

        template <class T>
            requires(
                is_express_layout_desire<std::ranges::range_value_t<T>>
                && is_nullable_like<typename std::ranges::range_value_t<T>::value_type>
            )
        [[nodiscard]] constexpr std::vector<std::size_t> where_null(T&& t)
        {
            std::vector<std::size_t> result;
            for (std::size_t i = 0; i < t.size(); ++i)
            {
                if (!(t[i].get().has_value()))
                {
                    result.push_back(i);
                }
            }
            return result;
        }

        template <class T>
        [[nodiscard]] constexpr std::array<std::size_t, 0> where_null(T&&) noexcept
        {
            return {};
        }

        template <class T>
            requires(is_plain_value_type<std::ranges::range_value_t<T>>)
        [[nodiscard]] constexpr decltype(auto) ensure_value_range(T&& t) noexcept
        {
            return std::forward<T>(t);
        }

        template <class T>
            requires(!is_plain_value_type<std::ranges::range_value_t<T>>)
        [[nodiscard]] constexpr decltype(auto) ensure_value_range(T&& t)
        {
            return t
                   | std::views::transform(
                       [](auto&& v)
                       {
                           return ensure_value(std::forward<decltype(v)>(v));
                       }
                   );
        }
    }  // namespace detail
}  // namespace sparrow
