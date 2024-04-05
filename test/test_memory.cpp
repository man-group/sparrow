#include <functional>

#include "sparrow/memory.hpp"

#include "doctest/doctest.h"

using namespace sparrow;

TEST_SUITE("value_ptr")
{
    TEST_CASE("constructor")
    {
        value_ptr<int> vp;
        CHECK(!vp);

        value_ptr vp1(42);
        REQUIRE(vp1);
        CHECK(*vp1 == 42);
    }

    TEST_CASE("copy constructor")
    {
        const value_ptr vp1(42);
        const value_ptr vp2(vp1);
        REQUIRE(vp1);
        REQUIRE(vp2);
        CHECK(*vp1 == 42);
        CHECK(*vp2 == 42);
    }

    TEST_CASE("copy")
    {
        const value_ptr vp1(42);
        const value_ptr vp2 = vp1;
        REQUIRE(vp1);
        REQUIRE(vp2);
        CHECK(*vp1 == 42);
        CHECK(*vp2 == 42);
    }

    TEST_CASE("move constructor")
    {
        value_ptr vp1(42);
        const value_ptr vp2(std::move(vp1));
        CHECK(!vp1);
        REQUIRE(vp2);
        CHECK(*vp2 == 42);
    }

    TEST_CASE("move assignment")
    {
        value_ptr vp1(42);
        value_ptr<int> vp2;
        vp2 = std::move(vp1);
        CHECK(!vp1);
        REQUIRE(vp2);
        CHECK(*vp2 == 42);

        value_ptr<int> vp3;
        {
            value_ptr vp4(43);
            vp3 = std::move(vp4);
        }
        REQUIRE(vp3);
        CHECK(*vp3 == 43);
    }

    TEST_CASE("operator*")
    {
        value_ptr vp(42);
        CHECK(*vp == 42);

        *vp = 43;
        CHECK(*vp == 43);
    }

    TEST_CASE("operator->")
    {
        value_ptr vp(std::vector<int>{42});
        CHECK(vp.operator->() == std::addressof(*vp));
        CHECK(vp->size() == 1);
        CHECK(vp->at(0) == 42);
    }

    TEST_CASE("operator bool")
    {
        value_ptr<int> vp;
        CHECK(!vp);

        value_ptr vp1(42);
        CHECK(vp1);
    }

    TEST_CASE("has_value")
    {
        value_ptr<int> vp;
        CHECK(!vp.has_value());

        value_ptr vp1(42);
        CHECK(vp1.has_value());
    }

    TEST_CASE("reset")
    {
        value_ptr vp(42);
        REQUIRE(vp);
        vp.reset();
        CHECK(!vp);
    }
}
