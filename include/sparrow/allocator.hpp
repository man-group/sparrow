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

#include <cstdint>
#include <memory>
#include <memory_resource>
#include <type_traits>
#include <variant>

namespace sparrow
{
    /*
     * allocator concept, based on the requirements specified by the standard:
     * https://en.cppreference.com/w/cpp/named_req/Allocator.
     */
    template <class T>
    concept allocator = std::copy_constructible<std::remove_cvref_t<T>>
        and std::move_constructible<std::remove_cvref_t<T>>
        and std::equality_comparable<std::remove_cvref_t<T>>
        and requires(std::remove_cvref_t<T>& alloc,
                     typename std::remove_cvref_t<T>::value_type* p,
                     std::size_t n)
        {
            typename std::remove_cvref_t<T>::value_type;
            { alloc.allocate(n) } -> std::same_as<typename std::remove_cvref_t<T>::value_type*>;
            { alloc.deallocate(p, n) } -> std::same_as<void>;
        };

    /*
     * Type erasure class for allocators. This allows to use any kind of allocator
     * (standard, polymorphic) without having to expose it as a template parameter.
     *
     * @tparam T value_type of the allocator
     * @tparam DA default allocator, instantiated when calling the default constructor
     */
    template <class T, class DA = std::allocator<T>>
    class any_allocator
    {
        template <class A>
        static constexpr bool is_value_stored = 
            std::is_same_v<A, std::allocator<T>> || std::is_same_v<A, std::pmr::polymorphic_allocator<T>>;

    public:

        using value_type = T;

        any_allocator()
            : m_storage(make_storage(DA()))
        {
        }

        any_allocator(const any_allocator& rhs)
            : m_storage(copy_storage(rhs.m_storage))
        {
        }

        any_allocator(any_allocator&&) = default;

        any_allocator& operator=(const any_allocator& rhs) = delete;
        any_allocator& operator=(any_allocator&& rhs) = delete;

        template <class A>
        any_allocator(A&& alloc)
        requires (not std::same_as<std::remove_cvref_t<A>, any_allocator>
                and sparrow::allocator<A>)
            : m_storage(make_storage(std::forward<A>(alloc)))
        {
        }

        [[nodiscard]] T* allocate(std::size_t n)
        {
            return std::visit([n](auto&& arg)
            {
                using A = std::decay_t<decltype(arg)>;
                if constexpr (is_value_stored<A>)
                    return arg.allocate(n);
                else
                    return arg->allocate(n);
            }, m_storage);
        }

        void deallocate(T* p, std::size_t n)
        {
            return std::visit([p, n](auto&& arg)
            {
                using A = std::decay_t<decltype(arg)>;
                if constexpr (is_value_stored<A>)
                    arg.deallocate(p, n);
                else
                    arg->deallocate(p, n);
            }, m_storage);
        }

        any_allocator select_on_container_copy_construction() const
        {
            return any_allocator(*this);
        }

        bool equal(const any_allocator& rhs) const
        {
            // YOLO!!
            return std::visit([&rhs](auto&& arg)
            {
                using A = std::decay_t<decltype(arg)>;
                if constexpr (is_value_stored<A>)
                {
                    return std::visit([&arg](auto&& arg2)
                    {
                        using A2 = std::decay_t<decltype(arg2)>;
                        if constexpr (is_value_stored<A2> && std::same_as<A, A2>)
                            return arg == arg2;
                        else
                            return false;
                    }, rhs.m_storage);
                }
                else
                {
                    return std::visit([&arg](auto&& arg2)
                    {
                        using A2 = std::decay_t<decltype(arg2)>;
                        if constexpr (is_value_stored<A2>)
                            return false;
                        else
                            return arg->equal(*arg2);
                    }, rhs.m_storage);
                }

            }, m_storage);
        }

    private:

        struct interface
        {
            [[nodiscard]] virtual T* allocate(std::size_t) = 0;
            virtual void deallocate(T*, std::size_t) = 0;
            virtual std::unique_ptr<interface> clone() const = 0;
            virtual bool equal(const interface&) const = 0;
            virtual ~interface() = default;
        };

        template <allocator A>
        struct impl : interface
        {
            A m_alloc;

            explicit impl(A alloc) : m_alloc(std::move(alloc)) {}

            [[nodiscard]] T* allocate(std::size_t n) override
            {
                return m_alloc.allocate(n);
            }

            void deallocate(T* p, std::size_t n) override
            {
                m_alloc.deallocate(p, n);
            }

            std::unique_ptr<interface> clone() const override
            {
                return std::make_unique<impl<A>>(m_alloc);
            }

            bool equal(const interface& rhs) const override
            {
                if (auto* p = dynamic_cast<const impl<A>*>(&rhs))
                {
                    return p->m_alloc == m_alloc;
                }
                return false;
            }
        };

        using storage_type = std::variant
        <
            std::allocator<T>,
            std::pmr::polymorphic_allocator<T>,
            std::unique_ptr<interface>
        >;

        template <class A>
        std::unique_ptr<interface> make_storage(A&& alloc) const
        {
            return std::make_unique<impl<std::decay_t<A>>>(std::forward<A>(alloc));
        }

        template <class A>
        requires is_value_stored<A>
        A&& make_storage(A&& alloc) const
        {
            return std::forward<A>(alloc);
        }

        storage_type copy_storage(const storage_type& rhs) const
        {
            return std::visit([](auto&& arg) -> storage_type
            {
                using A = std::decay_t<decltype(arg)>;
                if constexpr (is_value_stored<A>)
                    return { A(arg) };
                else
                    return { arg->clone() };
            }, rhs);
        }

        storage_type m_storage;
    };

    template <class T, class DA>
    bool operator==(const any_allocator<T, DA>& lhs, const any_allocator<T, DA>& rhs)
    {
        return lhs.equal(rhs);
    }
}

