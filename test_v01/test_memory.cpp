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

#include "sparrow_v01/utils/memory.hpp"

#include "doctest/doctest.h"

namespace sparrow::cloning_test
{
    class mock_base
    {
    public:

        virtual ~mock_base() = default;

        mock_base(mock_base&&) = delete;
        mock_base& operator=(const mock_base&) = delete;
        mock_base& operator=(mock_base&&) = delete;

        virtual mock_base* clone() const = 0;

    protected:

        mock_base() = default;
        mock_base(const mock_base&) = default;
    };

    class mock_derived : public mock_base
    {
    public:

        mock_derived()
        {
            ++m_instance_count;
        }

        virtual ~mock_derived()
        {
            --m_instance_count;
        }

        mock_derived* clone() const override
        {
            return new mock_derived(*this);
        }

        static int instance_count() { return m_instance_count; };

    private:

        mock_derived(const mock_derived&)
        {
            ++m_instance_count;
        }

        static int m_instance_count;
    };

    int mock_derived::m_instance_count = 0;
}

namespace sparrow
{
    TEST_SUITE("cloning_ptr")
    {
        using cloning_test::mock_base;
        using cloning_test::mock_derived;

        TEST_CASE("constructor")
        {
            SUBCASE("default")
            {
                cloning_ptr<mock_base> p1;
                CHECK_EQ(p1.get(), nullptr);
            }

            SUBCASE("from nullptr")
            {
                cloning_ptr<mock_base> p2(nullptr);
                CHECK_EQ(p2.get(), nullptr);
            }

            SUBCASE("from allocated pointer")
            {
                CHECK_EQ(mock_derived::instance_count(), 0);
                {
                    auto d = new mock_derived();
                    cloning_ptr<mock_base> p(d);
                    CHECK_EQ(p.get(), d);
                    CHECK_EQ(mock_derived::instance_count(), 1);
                }
                CHECK_EQ(mock_derived::instance_count(), 0);
            }
        }

        TEST_CASE("copy constructor")
        {
            SUBCASE("default")
            {
                cloning_ptr<mock_base> p1(new mock_derived());
                cloning_ptr<mock_base> p2(p1);
                CHECK_EQ(mock_derived::instance_count(), 2);
                CHECK_NE(p1.get(), p2.get());
            }

            SUBCASE("with conversion")
            {
                cloning_ptr<mock_derived> p1(new mock_derived());
                cloning_ptr<mock_base> p2(p1);
                CHECK_EQ(mock_derived::instance_count(), 2);
                CHECK_NE(p1.get(), p2.get());
            }
        }

        TEST_CASE("move constructor")
        {
            SUBCASE("default")
            {
                auto d = new mock_derived();
                cloning_ptr<mock_base> p1(d);
                cloning_ptr<mock_base> p2(std::move(p1));
                CHECK_EQ(mock_derived::instance_count(), 1);
                CHECK_EQ(p2.get(), d);
                CHECK_EQ(p1.get(), nullptr);
            }

            SUBCASE("with conversion")
            {
                auto d = new mock_derived();
                cloning_ptr<mock_derived> p1(d);
                cloning_ptr<mock_base> p2(std::move(p1));
                CHECK_EQ(mock_derived::instance_count(), 1);
                CHECK_EQ(p2.get(), d);
                CHECK_EQ(p1.get(), nullptr);
            }
        }

        TEST_CASE("copy assign")
        {
            SUBCASE("default")
            {
                auto d = new mock_derived();
                cloning_ptr<mock_derived> p1(d);
                cloning_ptr<mock_derived> p2(new mock_derived());
                CHECK_EQ(mock_derived::instance_count(), 2);
                p1 = p2;
                CHECK_EQ(mock_derived::instance_count(), 2);
                CHECK_NE(p1.get(), p2.get());
                CHECK_NE(p1.get(), d);
            }

            SUBCASE("with conversion")
            {
                auto d = new mock_derived();
                cloning_ptr<mock_base> p1(d);
                cloning_ptr<mock_derived> p2(new mock_derived());
                CHECK_EQ(mock_derived::instance_count(), 2);
                p1 = p2;
                CHECK_EQ(mock_derived::instance_count(), 2);
                CHECK_NE(p1.get(), p2.get());
                CHECK_NE(p1.get(), d);
            }

            SUBCASE("from nullptr")
            {
                auto d = new mock_derived();
                cloning_ptr<mock_derived> p(d);
                CHECK_EQ(mock_derived::instance_count(), 1);
                p = nullptr;
                CHECK_EQ(mock_derived::instance_count(), 0);
                CHECK_EQ(p.get(), nullptr);
            }
        }

        TEST_CASE("move assign")
        {
            SUBCASE("default")
            {
                auto d = new mock_derived();
                cloning_ptr<mock_derived> p1(d);
                cloning_ptr<mock_derived> p2(new mock_derived());
                CHECK_EQ(mock_derived::instance_count(), 2);
                p1 = std::move(p2);
                CHECK_EQ(mock_derived::instance_count(), 1);
                CHECK_NE(p1.get(), p2.get());
                CHECK_NE(p1.get(), d);
                CHECK_EQ(p2.get(), nullptr);
            }

            SUBCASE("with conversion")
            {
                auto d = new mock_derived();
                cloning_ptr<mock_base> p1(d);
                cloning_ptr<mock_derived> p2(new mock_derived());
                CHECK_EQ(mock_derived::instance_count(), 2);
                p1 = std::move(p2);
                CHECK_EQ(mock_derived::instance_count(), 1);
                CHECK_NE(p1.get(), p2.get());
                CHECK_NE(p1.get(), d);
                CHECK_EQ(p2.get(), nullptr);
            }
        }

        TEST_CASE("release")
        {
            auto d = new mock_derived();
            cloning_ptr<mock_derived> p(d);
            CHECK_EQ(mock_derived::instance_count(), 1);
            auto d2 = p.release();
            CHECK_EQ(mock_derived::instance_count(), 1);
            CHECK_EQ(d, d2);
            CHECK_EQ(p.get(), nullptr);
            delete d2;
        }

        TEST_CASE("reset")
        {
            auto d1 = new mock_derived();
            auto d2 = new mock_derived();
            cloning_ptr<mock_derived> p(d1);
            p.reset(d2);
            CHECK_EQ(p.get(), d2);
            CHECK_EQ(mock_derived::instance_count(), 1);
        }

        TEST_CASE("swap")
        {
            auto d1 = new mock_derived();
            auto d2 = new mock_derived();
            cloning_ptr<mock_derived> p1(d1);
            cloning_ptr<mock_derived> p2(d2);
            
            SUBCASE("method")
            {
                p1.swap(p2);
                CHECK_EQ(p1.get(), d2);
                CHECK_EQ(p2.get(), d1);
            }

            SUBCASE("free function")
            {
                swap(p1, p2);
                CHECK_EQ(p1.get(), d2);
                CHECK_EQ(p2.get(), d1);
            }
        }

        TEST_CASE("operator bool")
        {
            cloning_ptr<mock_derived> p;
            CHECK(!p);

            p.reset(new mock_derived());
            CHECK(p);
        }

        TEST_CASE("operator*")
        {
            auto d = new mock_derived();
            cloning_ptr<mock_derived> p(d);
            auto& unref = *p;
            CHECK_EQ(&unref, d);
        }

        TEST_CASE("operator->")
        {
            auto d = new mock_derived();
            cloning_ptr<mock_derived> p(d);
            auto d2 = p.operator->();
            CHECK_EQ(d2, d);
        }

        TEST_CASE("comparison")
        {
            auto d1 = new mock_derived();
            auto d2 = new mock_derived();
            cloning_ptr<mock_derived> p1(d1);
            cloning_ptr<mock_derived> p2(d2);
            cloning_ptr<mock_derived> p3(d1);
            cloning_ptr<mock_derived> p4;

            SUBCASE("equality")
            {
                CHECK(p1 == p3);
                CHECK(p1 == p1);
                CHECK(p1 != p2);
                CHECK(p1 != nullptr);
                CHECK(p4 == nullptr);
            }
            
            SUBCASE("ordering")
            {
                CHECK(p1 <= p1);
                CHECK(p1 >= p1);
                if (d1 < d2)
                {
                    CHECK(p1 < p2);
                    CHECK(p1 <= p2);
                    CHECK(p2 > p1);
                    CHECK(p2 >= p1);
                }
                else
                {
                    CHECK(p2 < p1);
                    CHECK(p2 <= p1);
                    CHECK(p1 > p2);
                    CHECK(p1 >= p2);
                }

                CHECK(p4 <= nullptr);
                CHECK(p4 >= nullptr);
                CHECK(p1 >= nullptr);
            }
            p3.release();
        }
    }
}
