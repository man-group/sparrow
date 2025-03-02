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

#include "sparrow/layout/temporal/time_array.hpp"
#include "sparrow/types/data_traits.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    using testing_types = mpl::rename<time_types_t, std::tuple>;

    template <typename T>
    std::vector<nullable<T>> make_nullable_values(size_t count)
    {
        std::vector<nullable<T>> values;
        values.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            values.push_back(nullable<T>(T(static_cast<T::duration::rep>(i))));
        }
        return values;
    }

    TEST_SUITE("time_array")
    {
        TEST_CASE_TEMPLATE_DEFINE("", T, time_array_id)
        {
            const auto input_values = make_nullable_values<T>(10);
            SUBCASE("constructors")
            {
                SUBCASE("with range")
                {
                    time_array<T> ar(input_values);
                    CHECK_EQ(ar.size(), input_values.size());
                }

                SUBCASE("copy")
                {
                    const time_array<T> ar(input_values);
                    const time_array<T> ar2(ar);
                    CHECK_EQ(ar, ar2);
                }

                SUBCASE("move")
                {
                    time_array<T> ar(input_values);
                    const time_array<T> ar2(std::move(ar));
                    CHECK_EQ(ar2.size(), input_values.size());
                }
            }

            SUBCASE("operator[]")
            {
                SUBCASE("const")
                {
                    const time_array<T> ar(input_values);
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        const auto ari = ar[i];
                        const auto input_values_i = input_values[i];
                        CHECK_EQ(ari, input_values_i);
                    }
                }

                SUBCASE("mutable")
                {
                    time_array<T> ar(input_values);

                    std::vector<nullable<T>> new_values = [&input_values]()
                    {
                        std::vector<nullable<T>> values;
                        values.reserve(input_values.size());
                        for (size_t i = 0; i < input_values.size(); ++i)
                        {
                            if constexpr (std::is_same_v<T, chrono::months>)
                            {
                                values.push_back(nullable<T>(T(i + 5)));
                            }
                            else if constexpr (std::is_same_v<T, days_time_interval>)
                            {
                                values.push_back(
                                    nullable<T>(T{std::chrono::days(i + 5), std::chrono::milliseconds(i + 5)})
                                );
                            }
                            else if constexpr (std::is_same_v<T, month_day_nanoseconds_interval>)
                            {
                                values.push_back(nullable<T>(
                                    T{std::chrono::months(i + 5),
                                      std::chrono::days(i + 5),
                                      std::chrono::nanoseconds(i + 5)}
                                ));
                            }
                            else if constexpr (std::is_same_v<T, chrono::time_seconds>)
                            {
                                values.push_back(nullable<T>(T(int32_t(i) + 5)));
                            }
                            else if constexpr (std::is_same_v<T, chrono::time_milliseconds>)
                            {
                                values.push_back(nullable<T>(T(int32_t(i) + 5)));
                            }
                            else if constexpr (std::is_same_v<T, chrono::time_microseconds>)
                            {
                                values.push_back(nullable<T>(T(int32_t(i) + 5)));
                            }
                            else if constexpr (std::is_same_v<T, chrono::time_nanoseconds>)
                            {
                                values.push_back(nullable<T>(T(int32_t(i) + 5)));
                            }
                        }
                        return values;
                    }();

                    CHECK_EQ(ar.size(), new_values.size());

                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        ar[i] = new_values[i];
                    }
                    for (size_t i = 0; i < ar.size(); ++i)
                    {
                        CHECK_EQ(ar[i], new_values[i]);
                    }
                }
            }

            SUBCASE("front")
            {
                SUBCASE("const")
                {
                    const time_array<T> ar(input_values);
                    CHECK_EQ(ar.front(), input_values.front());
                }
            }

            SUBCASE("back")
            {
                SUBCASE("const")
                {
                    const time_array<T> ar(input_values);
                    CHECK_EQ(ar.back(), input_values.back());
                }
            }

            SUBCASE("value_iterator")
            {
                time_array<T> ar(input_values);
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
                        CHECK_EQ(*iter, input_values[i].get());
                        ++iter;
                    }
                    CHECK_EQ(iter, ar_values.end());
                }
            }

            SUBCASE("const_value_iterator")
            {
                time_array<T> ar(input_values);
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
                        CHECK_EQ(*citer, input_values[i].get());
                        ++citer;
                    }
                    CHECK_EQ(citer, ar_values.end());
                }
            }

            SUBCASE("iterator")
            {
                time_array<T> ar(input_values);
                auto it = ar.begin();
                const auto end = ar.end();
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    CHECK_EQ(*it, input_values[i]);
                    ++it;
                }
                CHECK_EQ(it, end);
            }

            SUBCASE("const iterator")
            {
                const time_array<T> ar(input_values);
                auto it = ar.cbegin();
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    CHECK_EQ(*it, input_values[i]);
                    ++it;
                }
                CHECK_EQ(it, ar.cend());
            }

            SUBCASE("reverse_iterator")
            {
                time_array<T> ar(input_values);
                auto it = ar.rbegin();
                CHECK_EQ(*it, *(ar.end() - 1));
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    const auto idx = ar.size() - 1 - i;
                    CHECK_EQ(*it, input_values[idx]);
                    ++it;
                }
                CHECK_EQ(it, ar.rend());
            }

            const auto new_value = nullable<T>(T(99));

            SUBCASE("resize")
            {
                time_array<T> ar(input_values);
                const size_t new_size = ar.size() + 2;
                ar.resize(ar.size() + 2, new_value);
                REQUIRE_EQ(ar.size(), new_size);
                for (size_t i = 0; i < ar.size() - 2; ++i)
                {
                    CHECK_EQ(ar[i], input_values[i]);
                }
                CHECK_EQ(ar[input_values.size()], new_value);
                CHECK_EQ(ar[input_values.size() + 1], new_value);
            }

            SUBCASE("insert")
            {
                SUBCASE("with pos and value")
                {
                    SUBCASE("at the beginning")
                    {
                        time_array<T> ar(input_values);
                        auto pos = ar.cbegin();
                        ar.insert(pos, new_value);
                        CHECK_EQ(ar[0], new_value);
                        for (size_t i = 0; i < ar.size() - 1; ++i)
                        {
                            CHECK_EQ(ar[i + 1], input_values[i]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        time_array<T> ar(input_values);
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.insert(pos, new_value);
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                        CHECK_EQ(ar[idx], new_value);
                        for (size_t i = idx; i < ar.size() - 1; ++i)
                        {
                            CHECK_EQ(ar[i + 1], input_values[i]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        time_array<T> ar(input_values);
                        auto pos = ar.cend();
                        ar.insert(pos, new_value);
                        for (size_t i = 0; i < ar.size() - 1; ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                        CHECK_EQ(ar[ar.size() - 1], new_value);
                    }
                }

                SUBCASE("with pos, count and value")
                {
                    SUBCASE("at the beginning")
                    {
                        time_array<T> ar(input_values);
                        auto pos = ar.cbegin();
                        ar.insert(pos, new_value, 2);
                        CHECK_EQ(ar[0], new_value);
                        CHECK_EQ(ar[1], new_value);
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            CHECK_EQ(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        time_array<T> ar(input_values);
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.insert(pos, new_value, 2);
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                        CHECK_EQ(ar[idx], new_value);
                        CHECK_EQ(ar[idx + 1], new_value);
                        for (size_t i = idx; i < ar.size() - 2; ++i)
                        {
                            CHECK_EQ(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        time_array<T> ar(input_values);
                        auto pos = ar.cend();
                        ar.insert(pos, new_value, 2);
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                        CHECK_EQ(ar[ar.size() - 2], new_value);
                        CHECK_EQ(ar[ar.size() - 1], new_value);
                    }
                }

                SUBCASE("with pos and range")
                {
                    SUBCASE("at the beginning")
                    {
                        time_array<T> ar(input_values);
                        std::vector<nullable<T>> new_values = {new_value, new_value};
                        auto pos = ar.cbegin();
                        ar.insert(pos, new_values);
                        CHECK_EQ(ar[0], new_value);
                        CHECK_EQ(ar[1], new_value);
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            CHECK_EQ(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        time_array<T> ar(input_values);
                        const std::vector<nullable<T>> new_values = {new_value, new_value};
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.insert(pos, new_values);
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                        CHECK_EQ(ar[idx], new_value);
                        CHECK_EQ(ar[idx + 1], new_value);
                        for (size_t i = idx; i < ar.size() - 2; ++i)
                        {
                            CHECK_EQ(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        time_array<T> ar(input_values);
                        std::vector<nullable<T>> new_values = {new_value, new_value};
                        auto pos = ar.cend();
                        ar.insert(pos, new_values);
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                        CHECK_EQ(ar[ar.size() - 2], new_value);
                        CHECK_EQ(ar[ar.size() - 1], new_value);
                    }
                }

                SUBCASE("with pos and initializer list")
                {
                    SUBCASE("at the beginning")
                    {
                        time_array<T> ar(input_values);
                        auto pos = ar.cbegin();
                        ar.insert(pos, {new_value, new_value});
                        CHECK_EQ(ar[0], new_value);
                        CHECK_EQ(ar[1], new_value);
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            CHECK_EQ(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        time_array<T> ar(input_values);
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.insert(pos, {new_value, new_value});
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                        CHECK_EQ(ar[idx], new_value);
                        CHECK_EQ(ar[idx + 1], new_value);
                        for (size_t i = idx; i < ar.size() - 2; ++i)
                        {
                            CHECK_EQ(ar[i + 2], input_values[i]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        time_array<T> ar(input_values);
                        auto pos = ar.cend();
                        ar.insert(pos, {new_value, new_value});
                        for (size_t i = 0; i < ar.size() - 2; ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                        CHECK_EQ(ar[ar.size() - 2], new_value);
                        CHECK_EQ(ar[ar.size() - 1], new_value);
                    }
                }
            }

            SUBCASE("erase")
            {
                SUBCASE("with pos")
                {
                    SUBCASE("at the beginning")
                    {
                        time_array<T> ar(input_values);
                        auto pos = ar.cbegin();
                        ar.erase(pos);
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i + 1]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        time_array<T> ar(input_values);
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.erase(pos);
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                        for (size_t i = idx; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i + 1]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        time_array<T> ar(input_values);
                        auto pos = ar.cend() - 1;
                        ar.erase(pos);
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                    }
                }

                SUBCASE("with iterators")
                {
                    SUBCASE("at the beginning")
                    {
                        time_array<T> ar(input_values);
                        auto pos = ar.cbegin();
                        ar.erase(pos, pos + 2);
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i + 2]);
                        }
                    }

                    SUBCASE("in the middle")
                    {
                        time_array<T> ar(input_values);
                        const size_t idx = input_values.size() / 2;
                        auto pos = sparrow::next(ar.cbegin(), idx);
                        ar.erase(pos, pos + 2);
                        for (size_t i = 0; i < idx; ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                        for (size_t i = idx; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i + 2]);
                        }
                    }

                    SUBCASE("at the end")
                    {
                        time_array<T> ar(input_values);
                        auto pos = ar.cend() - 2;
                        ar.erase(pos, ar.cend());
                        for (size_t i = 0; i < ar.size(); ++i)
                        {
                            CHECK_EQ(ar[i], input_values[i]);
                        }
                    }
                }
            }

            SUBCASE("push_back")
            {
                time_array<T> ar(input_values);
                ar.push_back(new_value);
                CHECK_EQ(ar.size(), input_values.size() + 1);
                CHECK_EQ(ar[ar.size() - 1], new_value);
            }

            SUBCASE("pop_back")
            {
                time_array<T> ar(input_values);
                ar.pop_back();
                CHECK_EQ(ar.size(), input_values.size() - 1);
                for (size_t i = 0; i < ar.size(); ++i)
                {
                    CHECK_EQ(ar[i], input_values[i]);
                }
            }

#if defined(__cpp_lib_format)
            SUBCASE("format")
            {
                const time_array<T> ar(input_values);
                const std::string formatted = std::format("{}", ar);
                const std::string expected = []()
                {
                    if constexpr (std::is_same_v<T, chrono::time_seconds>)
                    {
                        return "Time seconds [name=nullptr | size=10] <0, 1, 2, 3, 4, 5, 6, 7, 8, 9>";
                    }
                    else if constexpr (std::is_same_v<T, chrono::time_milliseconds>)
                    {
                        return "Time milliseconds [name=nullptr | size=10] <0, 1, 2, 3, 4, 5, 6, 7, 8, 9>";
                    }
                    else if constexpr (std::is_same_v<T, chrono::time_microseconds>)
                    {
                        return "Time microseconds [name=nullptr | size=10] <0, 1, 2, 3, 4, 5, 6, 7, 8, 9>";
                    }
                    else if constexpr (std::is_same_v<T, chrono::time_nanoseconds>)
                    {
                        return "Time nanoseconds [name=nullptr | size=10] <0, 1, 2, 3, 4, 5, 6, 7, 8, 9>";
                    }
                }();
                CHECK_EQ(formatted, expected);
            }
#endif
        }
        TEST_CASE_TEMPLATE_APPLY(time_array_id, testing_types);
    }
}
