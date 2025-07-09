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

#include <cstdio>

#include "sparrow/timestamp_array.hpp"
#include "sparrow/utils/mp_utils.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    using testing_types = mpl::rename<timestamp_types_t, std::tuple>;

    static const date::time_zone* new_york = date::locate_zone("America/New_York");

    template <typename T>
    T make_value(size_t i)
    {
        using duration = T::duration;
        const duration duration_v{static_cast<int>(i)};
        const date::sys_time<duration> sys_time{duration_v};
        return T(new_york, sys_time);
    }

    template <typename T>
    std::vector<nullable<T>> make_nullable_values(size_t count)
    {
        std::vector<nullable<T>> values;
        values.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            values.push_back(nullable<T>(make_value<T>(i), i % 2));
        }
        return values;
    }

    template <typename T1, typename T2>
    void compare_timestamp(const T1& lhs, const T2& rhs)
    {
        if constexpr (mpl::is_type_instance_of_v<T1, nullable> && mpl::is_type_instance_of_v<T2, nullable>)
        {
            if (!lhs.has_value() || !rhs.has_value())
            {
                CHECK_EQ(lhs.has_value(), rhs.has_value());
                return;
            }
            if constexpr (mpl::is_type_instance_of_v<typename T1::value_type, timestamp_reference>
                          && !mpl::is_type_instance_of_v<typename T2::value_type, timestamp_reference>)
            {
                CHECK_EQ(
                    lhs.get().value().get_sys_time().time_since_epoch(),
                    rhs.get().get_sys_time().time_since_epoch()
                );
                return;
            }
            if constexpr (!mpl::is_type_instance_of_v<typename T1::value_type, timestamp_reference>
                          && mpl::is_type_instance_of_v<typename T2::value_type, timestamp_reference>)
            {
                CHECK_EQ(
                    lhs.get().get_sys_time().time_since_epoch(),
                    rhs.get().value().get_sys_time().time_since_epoch()
                );
                return;
            }
            if constexpr (mpl::is_type_instance_of_v<typename T1::value_type, timestamp_reference>
                          && mpl::is_type_instance_of_v<typename T2::value_type, timestamp_reference>)
            {
                CHECK_EQ(
                    lhs.get().value().get_sys_time().time_since_epoch(),
                    rhs.get().value().get_sys_time().time_since_epoch()
                );
                return;
            }
        }
        else
        {
            CHECK_EQ(lhs.get_sys_time().time_since_epoch(), rhs.get_sys_time().time_since_epoch());
        }
    }

    TEST_SUITE("timestamp_array")
    {
        TEST_CASE_TEMPLATE_DEFINE("", T, timestamp_array_id)
        {
            const auto input_values = make_nullable_values<T>(10);
            SUBCASE("constructors")
            {
                SUBCASE("with range")
                {
                    timestamp_array<T> ar(new_york, input_values);
                    CHECK_EQ(ar.size(), input_values.size());
                }

                SUBCASE("copy")
                {
                    const timestamp_array<T> ar(new_york, input_values);
                    const timestamp_array<T> ar2(ar);
                    CHECK_EQ(ar, ar2);
                }

                SUBCASE("move")
                {
                    timestamp_array<T> ar(new_york, input_values);
                    const timestamp_array<T> ar2(std::move(ar));
                    CHECK_EQ(ar2.size(), input_values.size());
                }
            }

            SUBCASE("operator[]")
            {
                SUBCASE("const")
                {
                    const timestamp_array<T> ar(new_york, input_values);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        compare_timestamp(ar[i], input_values[i]);
                    }
                }

                SUBCASE("mutable")
                {
                    timestamp_array<T> ar(new_york, input_values);

                    std::vector<nullable<T>> new_values = [&input_values]()
                    {
                        std::vector<nullable<T>> values;
                        values.reserve(input_values.size());
                        for (size_t i = 0; i < input_values.size(); ++i)
                        {
                            values.push_back(nullable<T>(make_value<T>(i + 5), i % 2));
                        }
                        return values;
                    }();

                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        ar[i] = new_values[i];
                    }
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        compare_timestamp(ar[i], new_values[i]);
                    }
                }
            }

            SUBCASE("front")
            {
                SUBCASE("const")
                {
                    const timestamp_array<T> ar(new_york, input_values);
                    compare_timestamp(ar.front(), input_values.front());
                }
            }

            SUBCASE("back")
            {
                SUBCASE("const")
                {
                    const timestamp_array<T> ar(new_york, input_values);
                    compare_timestamp(ar.back(), input_values.back());
                }
            }

            SUBCASE("value_iterator")
            {
                timestamp_array<T> ar(new_york, input_values);
                auto ar_values = ar.values();
                SUBCASE("ordering")
                {
                    auto iter = ar_values.begin();
                    CHECK(iter < ar_values.end());
                }

                SUBCASE("equality")
                {
                    auto iter = ar_values.begin();
                    for (size_t i = 0; i < ar_values.size(); ++i)
                    {
                        compare_timestamp(*iter, input_values[i].get());
                        ++iter;
                    }
                    CHECK_EQ(iter, ar_values.end());
                }
            }

            SUBCASE("const_value_iterator")
            {
                timestamp_array<T> ar(new_york, input_values);
                auto ar_values = ar.values();

                SUBCASE("ordering")
                {
                    const auto citer = ar_values.begin();
                    CHECK(citer < ar_values.end());
                }

                SUBCASE("equality")
                {
                    auto citer = ar_values.begin();
                    for (size_t i = 0; i < ar_values.size(); ++i)
                    {
                        compare_timestamp(*citer, input_values[i].get());
                        ++citer;
                    }
                    CHECK_EQ(citer, ar_values.end());
                }
            }

            SUBCASE("iterator")
            {
                timestamp_array<T> ar(new_york, input_values);
                auto it = ar.begin();
                const auto end = ar.end();
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    compare_timestamp(*it, input_values[i]);
                    ++it;
                }
                CHECK_EQ(it, end);
            }

            SUBCASE("const iterator")
            {
                const timestamp_array<T> ar(new_york, input_values);
                auto it = ar.cbegin();
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    compare_timestamp(*it, input_values[i]);
                    ++it;
                }
                CHECK_EQ(it, ar.cend());
            }

            SUBCASE("reverse_iterator")
            {
                timestamp_array<T> ar(new_york, input_values);
                auto it = ar.rbegin();
                compare_timestamp(*it, *(ar.end() - 1));
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    const auto idx = ar.size() - 1 - i;
                    compare_timestamp(*it, input_values[idx]);
                    ++it;
                }
                CHECK_EQ(it, ar.rend());
            }

            SUBCASE("resize")
            {
                timestamp_array<T> ar(new_york, input_values);
                const auto new_value = make_nullable<T>(make_value<T>(99));
                const size_t new_size = ar.size() + 2;
                ar.resize(ar.size() + 2, new_value);
                REQUIRE_EQ(ar.size(), new_size);
                for (size_t i = 0; i < ar.size() - 2; ++i)
                {
                    compare_timestamp(ar[i], input_values[i]);
                }
                compare_timestamp(ar[input_values.size()], new_value);
                compare_timestamp(ar[input_values.size() + 1], new_value);
            }

            SUBCASE("insert")
            {
                SUBCASE("with pos and value")
                {
                    SUBCASE("at the beginning")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        auto pos = ar.cbegin();
                        ar.insert(pos, new_value);
                        compare_timestamp(ar[0], new_value);
                        for (size_t i = 0; i < ar.size() - 1; ++i)
                        {
                            compare_timestamp(ar[i + 1], input_values[i]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.insert(pos, new_value);
                        for (size_t i = 0; i < idx; ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                        compare_timestamp(ar[idx], new_value);
                        for (size_t i = idx; i < ar.size() - 1; ++i)
                        {
                            compare_timestamp(ar[i + 1], input_values[i]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        auto pos = ar.cend();
                        ar.insert(pos, new_value);
                        for (size_t i = 0; i < ar.size() - 1; ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                        compare_timestamp(ar[ar.size() - 1], new_value);
                    }
                }

                SUBCASE("with pos, count and value")
                {
                    SUBCASE("at the beginning")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        auto pos = ar.cbegin();
                        ar.insert(pos, new_value, 2);
                        compare_timestamp(ar[0], new_value);
                        compare_timestamp(ar[1], new_value);
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            compare_timestamp(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.insert(pos, new_value, 2);
                        for (size_t i = 0; i < idx; ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                        compare_timestamp(ar[idx], new_value);
                        compare_timestamp(ar[idx + 1], new_value);
                        for (size_t i = idx; i < ar.size() - 2; ++i)
                        {
                            compare_timestamp(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        auto pos = ar.cend();
                        ar.insert(pos, new_value, 2);
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                        compare_timestamp(ar[ar.size() - 2], new_value);
                        compare_timestamp(ar[ar.size() - 1], new_value);
                    }
                }

                SUBCASE("with pos and range")
                {
                    SUBCASE("at the beginning")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        std::vector<nullable<T>> new_values = {new_value, new_value};
                        auto pos = ar.cbegin();
                        ar.insert(pos, new_values);
                        compare_timestamp(ar[0], new_value);
                        compare_timestamp(ar[1], new_value);
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            compare_timestamp(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        const std::vector<nullable<T>> new_values = {new_value, new_value};
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.insert(pos, new_values);
                        for (size_t i = 0; i < idx; ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                        compare_timestamp(ar[idx], new_value);
                        compare_timestamp(ar[idx + 1], new_value);
                        for (size_t i = idx; i < ar.size() - 2; ++i)
                        {
                            compare_timestamp(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        std::vector<nullable<T>> new_values = {new_value, new_value};
                        auto pos = ar.cend();
                        ar.insert(pos, new_values);
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                        compare_timestamp(ar[ar.size() - 2], new_value);
                        compare_timestamp(ar[ar.size() - 1], new_value);
                    }
                }

                SUBCASE("with pos and initializer list")
                {
                    SUBCASE("at the beginning")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        auto pos = ar.cbegin();
                        ar.insert(pos, {new_value, new_value});
                        compare_timestamp(ar[0], new_value);
                        compare_timestamp(ar[1], new_value);
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            compare_timestamp(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.insert(pos, {new_value, new_value});
                        for (size_t i = 0; i < idx; ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                        compare_timestamp(ar[idx], new_value);
                        compare_timestamp(ar[idx + 1], new_value);
                        for (size_t i = idx; i < ar.size() - 2; ++i)
                        {
                            compare_timestamp(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const auto new_value = make_nullable<T>(make_value<T>(99));
                        auto pos = ar.cend();
                        ar.insert(pos, {new_value, new_value});
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                        compare_timestamp(ar[ar.size() - 2], new_value);
                        compare_timestamp(ar[ar.size() - 1], new_value);
                    }
                }
            }

            SUBCASE("erase")
            {
                SUBCASE("with pos")
                {
                    SUBCASE("at the beginning")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        auto pos = ar.cbegin();
                        ar.erase(pos);
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            compare_timestamp(ar[i], input_values[i + 1]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.erase(pos);
                        for (size_t i = 0; i < idx; ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                        for (size_t i = idx; i < ar.size(); ++i)
                        {
                            compare_timestamp(ar[i], input_values[i + 1]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        auto pos = ar.cend() - 1;
                        ar.erase(pos);
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                    }
                }

                SUBCASE("with iterators")
                {
                    SUBCASE("at the beginning")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        auto pos = ar.cbegin();
                        ar.erase(pos, pos + 2);
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            compare_timestamp(ar[i], input_values[i + 2]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.erase(pos, pos + 2);
                        for (size_t i = 0; i < idx; ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                        for (size_t i = idx; i < ar.size(); ++i)
                        {
                            compare_timestamp(ar[i], input_values[i + 2]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        timestamp_array<T> ar(new_york, input_values);
                        auto pos = ar.cend() - 2;
                        ar.erase(pos, ar.cend());
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            compare_timestamp(ar[i], input_values[i]);
                        }
                    }
                }
            }

            SUBCASE("push_back")
            {
                timestamp_array<T> ar(new_york, input_values);
                const auto new_value = make_nullable<T>(make_value<T>(99));
                ar.push_back(new_value);
                CHECK_EQ(ar.size(), input_values.size() + 1);
                compare_timestamp(ar[ar.size() - 1], new_value);
            }

            SUBCASE("pop_back")
            {
                timestamp_array<T> ar(new_york, input_values);
                ar.pop_back();
                CHECK_EQ(ar.size(), input_values.size() - 1);
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    compare_timestamp(ar[i], input_values[i]);
                }
            }

            SUBCASE("zero_null_values")
            {
                timestamp_array<T> ar(new_york, input_values);
                ar.zero_null_values();
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    if (!input_values[i].has_value())
                    {
                        compare_timestamp(ar[i].get().value(), make_value<T>(0));
                    }
                }
            }
        }
        TEST_CASE_TEMPLATE_APPLY(timestamp_array_id, testing_types);

        // TODO: Fix on exotic architectures
        // #if defined(SPARROW_USE_DATE_POLYFILL)
        // #    if defined(__cpp_lib_format)
        //         TEST_CASE("formatting")
        //         {
        //             std::vector<timestamp_second> values{
        //                 timestamp_second{new_york, date::sys_days{date::year{2022} / 2 / 1}},
        //                 timestamp_second{new_york, date::sys_days{date::year{2022} / 3 / 2}},
        //                 timestamp_second{new_york, date::sys_days{date::year{2022} / 4 / 3}}
        //             };
        //             timestamp_seconds_array ar(new_york, values);
        //             const std::string
        //                 expected = R"(Timestamp seconds [name=nullptr | size=3] <2022-01-31 20:00:00 EDT,
        //                 2022-03-01 20:00:00 EDT, 2022-04-02 20:00:00 EDT>)";
        //             CHECK_EQ(std::format("{}", ar), expected);
        //         }
        // #    endif
        // #endif
    }
}  // namespace sparrow
