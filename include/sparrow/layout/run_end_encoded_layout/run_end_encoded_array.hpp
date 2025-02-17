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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "sparrow/array_api.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/run_end_encoded_layout/run_end_encoded_iterator.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{
    class run_end_encoded_array;

    /**
     * Checks whether T is a run_end_encoded_array type.
     */
    template <class T>
    constexpr bool is_run_end_encoded_array_v = std::same_as<T, run_end_encoded_array>;

    namespace detail
    {
        template <class T>
        struct get_data_type_from_array;

        template <>
        struct get_data_type_from_array<sparrow::run_end_encoded_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::RUN_ENCODED;
            }
        };
    }

    class run_end_encoded_array
    {
    public:

        using self_type = run_end_encoded_array;
        using size_type = std::size_t;
        using inner_value_type = array_traits::inner_value_type;
        using iterator = run_encoded_array_iterator<false>;
        using const_iterator = run_encoded_array_iterator<true>;

        SPARROW_API explicit run_end_encoded_array(arrow_proxy proxy);

        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<run_end_encoded_array, Args...>)
        explicit run_end_encoded_array(Args&&... args)
            : run_end_encoded_array(create_proxy(std::forward<Args>(args)...))
        {
        }

        SPARROW_API run_end_encoded_array(const self_type&);
        SPARROW_API self_type& operator=(const self_type&);

        run_end_encoded_array(self_type&&) = default;
        self_type& operator=(self_type&&) = default;

        [[nodiscard]] SPARROW_API array_traits::const_reference operator[](std::uint64_t i);
        [[nodiscard]] SPARROW_API array_traits::const_reference operator[](std::uint64_t i) const;

        [[nodiscard]] SPARROW_API iterator begin();
        [[nodiscard]] SPARROW_API iterator end();

        [[nodiscard]] SPARROW_API const_iterator begin() const;
        [[nodiscard]] SPARROW_API const_iterator end() const;

        [[nodiscard]] SPARROW_API const_iterator cbegin() const;
        [[nodiscard]] SPARROW_API const_iterator cend() const;

        [[nodiscard]] SPARROW_API array_traits::const_reference front() const;
        [[nodiscard]] SPARROW_API array_traits::const_reference back() const;

        [[nodiscard]] SPARROW_API bool empty() const;
        [[nodiscard]] SPARROW_API size_type size() const;

        [[nodiscard]] std::optional<std::string_view> name() const;
        [[nodiscard]] std::optional<KeyValueView> metadata() const;

    private:

        [[nodiscard]] SPARROW_API static auto create_proxy(
            array&& acc_lengths,
            array&& encoded_values,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<std::string_view> metadata = std::nullopt
        ) -> arrow_proxy;

        using acc_length_ptr_variant_type = std::variant<const std::uint16_t*, const std::uint32_t*, const std::uint64_t*>;

        [[nodiscard]] SPARROW_API static std::pair<std::int64_t, std::int64_t>
        extract_length_and_null_count(const array&, const array&);
        [[nodiscard]] SPARROW_API static acc_length_ptr_variant_type
        get_acc_lengths_ptr(const array_wrapper& ar);
        [[nodiscard]] SPARROW_API std::uint64_t get_run_length(std::uint64_t run_index) const;

        [[nodiscard]] SPARROW_API arrow_proxy& get_arrow_proxy();
        [[nodiscard]] SPARROW_API const arrow_proxy& get_arrow_proxy() const;

        arrow_proxy m_proxy;
        std::uint64_t m_encoded_length;

        cloning_ptr<array_wrapper> p_acc_lengths_array;
        cloning_ptr<array_wrapper> p_encoded_values_array;
        acc_length_ptr_variant_type m_acc_lengths;

        // friend classes
        friend class run_encoded_array_iterator<false>;
        friend class run_encoded_array_iterator<true>;
        friend class detail::array_access;
    };

    SPARROW_API
    bool operator==(const run_end_encoded_array& lhs, const run_end_encoded_array& rhs);
}  // namespace sparrow

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::run_end_encoded_array>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    SPARROW_API auto format(const sparrow::run_end_encoded_array& ar, std::format_context& ctx) const
        -> decltype(ctx.out());
};

namespace sparrow
{
    SPARROW_API std::ostream& operator<<(std::ostream& os, const sparrow::run_end_encoded_array& value);
}

#endif
