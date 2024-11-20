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
#include "sparrow/array_factory.hpp"
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
            static constexpr sparrow::data_type get()
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

        SPARROW_API run_end_encoded_array(self_type&&) = default;
        SPARROW_API self_type& operator=(self_type&&) = default;

        SPARROW_API array_traits::const_reference operator[](std::uint64_t i);
        SPARROW_API array_traits::const_reference operator[](std::uint64_t i) const;

        SPARROW_API iterator begin();
        SPARROW_API iterator end();

        SPARROW_API const_iterator begin() const;
        SPARROW_API const_iterator end() const;

        SPARROW_API const_iterator cbegin() const;
        SPARROW_API const_iterator cend() const;

        SPARROW_API bool empty() const;
        SPARROW_API size_type size() const;

        SPARROW_API array_traits::const_reference front() const;
        SPARROW_API array_traits::const_reference back() const;

    private:

        SPARROW_API static auto create_proxy(array&& acc_lengths, array&& encoded_values) -> arrow_proxy;

        using acc_length_ptr_variant_type = std::variant<const std::uint16_t*, const std::uint32_t*, const std::uint64_t*>;

        SPARROW_API static std::pair<std::int64_t, std::int64_t>
        extract_length_and_null_count(const array&, const array&);
        SPARROW_API static acc_length_ptr_variant_type get_acc_lengths_ptr(const array_wrapper& ar);
        SPARROW_API std::uint64_t get_run_length(std::uint64_t run_index) const;

        [[nodiscard]] arrow_proxy& get_arrow_proxy();
        [[nodiscard]] const arrow_proxy& get_arrow_proxy() const;

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

    /****************************************
     * run_end_encoded_array implementation *
     ****************************************/

    inline run_end_encoded_array::run_end_encoded_array(arrow_proxy proxy)
        : m_proxy(std::move(proxy))
        , m_encoded_length(m_proxy.children()[0].length())
        , p_acc_lengths_array(array_factory(m_proxy.children()[0].view()))
        , p_encoded_values_array(array_factory(m_proxy.children()[1].view()))
        , m_acc_lengths(run_end_encoded_array::get_acc_lengths_ptr(*p_acc_lengths_array))
    {
    }

    inline run_end_encoded_array::run_end_encoded_array(const self_type& rhs)
        : run_end_encoded_array(rhs.m_proxy)
    {
    }

    inline auto run_end_encoded_array::operator=(const self_type& rhs) -> self_type&
    {
        if (this != &rhs)
        {
            m_proxy = rhs.m_proxy;
            m_encoded_length = rhs.m_encoded_length;
            p_acc_lengths_array = array_factory(m_proxy.children()[0].view());
            p_encoded_values_array = array_factory(m_proxy.children()[1].view());
            m_acc_lengths = run_end_encoded_array::get_acc_lengths_ptr(*p_acc_lengths_array);
        }
        return *this;
    }

    inline auto run_end_encoded_array::size() const -> size_type
    {
        return m_proxy.length();
    }

    inline auto run_end_encoded_array::empty() const -> bool
    {
        return size() == 0;
    }

    inline auto run_end_encoded_array::get_run_length(std::uint64_t run_index) const -> std::uint64_t
    {
        auto ret = std::visit(
            [run_index](auto&& acc_lengths_ptr) -> std::uint64_t
            {
                if (run_index == 0)
                {
                    return static_cast<std::uint64_t>(acc_lengths_ptr[run_index]);
                }
                else
                {
                    return static_cast<std::uint64_t>(
                        acc_lengths_ptr[run_index] - acc_lengths_ptr[run_index - 1]
                    );
                }
            },
            m_acc_lengths
        );
        return ret;
    }

    inline arrow_proxy& run_end_encoded_array::get_arrow_proxy()
    {
        return m_proxy;
    }

    inline const arrow_proxy& run_end_encoded_array::get_arrow_proxy() const
    {
        return m_proxy;
    }

    inline auto run_end_encoded_array::operator[](std::uint64_t i) -> array_traits::const_reference
    {
        return static_cast<const run_end_encoded_array*>(this)->operator[](i);
    }

    inline auto run_end_encoded_array::begin() -> iterator
    {
        return iterator(this, 0, 0);
    }

    inline auto run_end_encoded_array::end() -> iterator
    {
        return iterator(this, size(), 0);
    }

    inline auto run_end_encoded_array::begin() const -> const_iterator
    {
        return this->cbegin();
    }

    inline auto run_end_encoded_array::end() const -> const_iterator
    {
        return this->cend();
    }

    inline auto run_end_encoded_array::cbegin() const -> const_iterator
    {
        return const_iterator(this, 0, 0);
    }

    inline auto run_end_encoded_array::cend() const -> const_iterator
    {
        return const_iterator(this, size(), 0);
    }

    inline auto run_end_encoded_array::front() const -> array_traits::const_reference
    {
        return operator[](0);
    }

    inline auto run_end_encoded_array::back() const -> array_traits::const_reference
    {
        return operator[](size() - 1);
    }

    inline bool operator==(const run_end_encoded_array& lhs, const run_end_encoded_array& rhs)
    {
        return std::ranges::equal(lhs, rhs);
    }
}  // namespace sparrow

#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::run_end_encoded_array>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    auto format(const sparrow::run_end_encoded_array& ar, std::format_context& ctx) const
    {
        std::format_to(ctx.out(), "Run end encoded [size={}] <", ar.size());

        std::for_each(
            ar.cbegin(),
            sparrow::next(ar.cbegin(), ar.size() - 1),
            [&ctx](const auto& value)
            {
                std::format_to(ctx.out(), "{}, ", value);
            }
        );

        return std::format_to(ctx.out(), "{}>", ar.back());
    }
};

#endif
