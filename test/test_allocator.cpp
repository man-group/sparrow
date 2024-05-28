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

#include <memory>
#include <memory_resource>
#include <numeric>
#include <vector>

#include "sparrow/allocator.hpp"

#include "doctest/doctest.h"

TEST_SUITE("any_allocator")
{
    TEST_CASE_TEMPLATE_DEFINE("value semantic", A, value_semantic_id)
    {
        SUBCASE("constructor")
        {
            {
                sparrow::any_allocator<int> a;
            }
            {
                A alloc;
                sparrow::any_allocator<typename A::value_type> a(alloc);
            }
        }

        SUBCASE("copy constructor")
        {
            using value_type = typename A::value_type;

            A alloc;
            sparrow::any_allocator<value_type> a(alloc);
            sparrow::any_allocator<value_type> b(a);
            CHECK(a == b);

            auto d = b.select_on_container_copy_construction();
            CHECK(d == b);
        }

        SUBCASE("move constructor")
        {
            using value_type = typename A::value_type;

            A alloc;
            sparrow::any_allocator<value_type> a(alloc);
            sparrow::any_allocator<value_type> aref(a);
            sparrow::any_allocator<value_type> b(std::move(a));
            CHECK_EQ(b, aref);
        }
    }

    TEST_CASE_TEMPLATE_DEFINE("allocate / deallocate", A, allocate_id)
    {
        using value_type = typename A::value_type;

        constexpr std::size_t n = 100;
        std::vector<value_type> ref(n);
        std::iota(ref.begin(), ref.end(), value_type());

        A alloc;
        sparrow::any_allocator<value_type> a(alloc);
        value_type* buf = a.allocate(n);
        std::uninitialized_copy(ref.cbegin(), ref.cend(), buf);
        CHECK_EQ(*buf, ref[0]);
        CHECK_EQ(*(buf + n - 1), ref.back());
        a.deallocate(buf, n);
    }

#if defined(__APPLE__)
    // /usr/lib/libc++.1.dylib is missing the symbol __ZNSt3__13pmr20get_default_resourceEv, leading
    // to an exception at runtime.
    TEST_CASE_TEMPLATE_INVOKE(
        value_semantic_id,
        std::allocator<int> /*, std::pmr::polymorphic_allocator<int>*/
    );
    TEST_CASE_TEMPLATE_INVOKE(allocate_id, std::allocator<int> /*, std::pmr::polymorphic_allocator<int>*/);
#else
    TEST_CASE_TEMPLATE_INVOKE(value_semantic_id, std::allocator<int>, std::pmr::polymorphic_allocator<int>);
    TEST_CASE_TEMPLATE_INVOKE(allocate_id, std::allocator<int>, std::pmr::polymorphic_allocator<int>);
#endif
}
