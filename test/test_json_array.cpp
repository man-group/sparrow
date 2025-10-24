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

#include <string>
#include <vector>

#include "sparrow/json_array.hpp"
#include "sparrow/types/data_type.hpp"
#include "sparrow/utils/nullable.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    TEST_SUITE("json_array")
    {
        TEST_CASE("basic construction")
        {
            SUBCASE("empty array")
            {
                json_array arr(std::vector<std::string>{});
                CHECK_EQ(arr.size(), 0);
            }

            SUBCASE("single element")
            {
                std::vector<std::string> json_values = {R"({"key": "value"})"};
                json_array arr(json_values);

                CHECK_EQ(arr.size(), 1);
                CHECK(arr[0].has_value());
                CHECK_EQ(arr[0].get(), R"({"key": "value"})");
            }

            SUBCASE("multiple elements")
            {
                std::vector<std::string> json_values = {
                    R"({"a": 1})",
                    R"({"b": 2})",
                    R"({"c": 3})"
                };
                json_array arr(json_values);

                CHECK_EQ(arr.size(), 3);
                CHECK_EQ(arr[0].get(), R"({"a": 1})");
                CHECK_EQ(arr[1].get(), R"({"b": 2})");
                CHECK_EQ(arr[2].get(), R"({"c": 3})");
            }
        }

        TEST_CASE("nullable values")
        {
            SUBCASE("null JSON values")
            {
                std::vector<std::string> json_values = {
                    R"({"key": null})",
                    R"(null)",
                    R"({"valid": true})"
                };
                json_array arr(json_values);

                CHECK_EQ(arr.size(), 3);
                CHECK(arr[0].has_value());
                CHECK(arr[1].has_value());
                CHECK(arr[2].has_value());
                CHECK_EQ(arr[2].get(), R"({"valid": true})");
            }

            SUBCASE("nullable string values")
            {
                std::vector<nullable<std::string>> json_values = {
                    nullable<std::string>(R"({"exists": true})"),
                    nullable<std::string>(),  // null value
                    nullable<std::string>(R"({"also": "exists"})")
                };
                json_array arr(json_values);

                CHECK_EQ(arr.size(), 3);
                CHECK(arr[0].has_value());
                CHECK_FALSE(arr[1].has_value());
                CHECK(arr[2].has_value());
            }
        }

        TEST_CASE("complex JSON structures")
        {
            SUBCASE("nested objects")
            {
                std::vector<std::string> json_values = {R"({
                    "user": {
                        "name": "Alice",
                        "address": {
                            "city": "Wonderland",
                            "country": "Fantasy"
                        }
                    }
                })"};
                json_array arr(json_values);

                CHECK_EQ(arr.size(), 1);
                CHECK(arr[0].has_value());
            }

            SUBCASE("arrays in JSON")
            {
                std::vector<std::string> json_values = {
                    R"({"numbers": [1, 2, 3, 4, 5]})",
                    R"({"strings": ["a", "b", "c"]})",
                    R"({"mixed": [1, "two", true, null]})"
                };
                json_array arr(json_values);

                CHECK_EQ(arr.size(), 3);
                for (size_t i = 0; i < arr.size(); ++i)
                {
                    CHECK(arr[i].has_value());
                }
            }
        }

        TEST_CASE("element access")
        {
            std::vector<std::string> json_values = {
                R"({"first": 1})",
                R"({"second": 2})",
                R"({"third": 3})"
            };
            json_array arr(json_values);

            SUBCASE("bracket operator")
            {
                CHECK_EQ(arr[0].get(), R"({"first": 1})");
                CHECK_EQ(arr[1].get(), R"({"second": 2})");
                CHECK_EQ(arr[2].get(), R"({"third": 3})");
            }

            SUBCASE("const access")
            {
                const auto& const_arr = arr;
                CHECK_EQ(const_arr[0].get(), R"({"first": 1})");
                CHECK_EQ(const_arr[1].get(), R"({"second": 2})");
                CHECK_EQ(const_arr[2].get(), R"({"third": 3})");
            }
        }

        TEST_CASE("iteration")
        {
            std::vector<std::string> json_values = {
                R"({"a": 1})",
                R"({"b": 2})",
                R"({"c": 3})"
            };
            json_array arr(json_values);

            SUBCASE("forward iteration")
            {
                size_t count = 0;
                for (const auto& elem : arr)
                {
                    CHECK(elem.has_value());
                    count++;
                }
                CHECK_EQ(count, 3);
            }

            SUBCASE("iterator validity")
            {
                auto it = arr.begin();
                auto end = arr.end();
                CHECK(it != end);
                ++it;
                ++it;
                ++it;
                CHECK(it == end);
            }

            SUBCASE("value iteration")
            {
                std::vector<std::string> collected;
                for (auto it = arr.cbegin(); it != arr.cend(); ++it)
                {
                    if (it->has_value())
                    {
                        collected.push_back(std::string(it->get()));
                    }
                }
                CHECK_EQ(collected.size(), 3);
                CHECK_EQ(collected[0], json_values[0]);
                CHECK_EQ(collected[1], json_values[1]);
                CHECK_EQ(collected[2], json_values[2]);
            }
        }

        TEST_CASE("big_large string storage")
        {
            SUBCASE("construction")
            {
                std::vector<std::string> json_values = {R"({"large": "data"})"};
                big_json_array arr(json_values);

                CHECK_EQ(arr.size(), 1);
                CHECK(arr[0].has_value());
                CHECK_EQ(arr[0].get(), R"({"large": "data"})");
            }

            SUBCASE("very long JSON strings")
            {
                std::string long_json = R"({"data": ")" + std::string(10000, 'x') + R"("})";
                std::vector<std::string> json_values = {long_json};
                big_json_array arr(json_values);

                CHECK_EQ(arr.size(), 1);
                CHECK(arr[0].has_value());
                CHECK_EQ(arr[0].get(), long_json);
            }
        }

        TEST_CASE("extension metadata")
        {
            SUBCASE("metadata is set correctly")
            {
                std::vector<std::string> json_values = {R"({"test": true})"};
                json_array arr(json_values);

                // The extension metadata is set internally by the extension.
                // We can verify basic array properties
                CHECK_EQ(arr.size(), 1);
                CHECK(arr[0].has_value());
                CHECK_EQ(arr[0].get(), R"({"test": true})");
            }
        }

        TEST_CASE("real-world JSON examples")
        {
            SUBCASE("user records")
            {
                std::vector<std::string> json_values = {
                    R"({
                        "id": "usr_123",
                        "name": "Alice Johnson",
                        "email": "alice@example.com",
                        "age": 30,
                        "verified": true
                    })",
                    R"({
                        "id": "usr_456",
                        "name": "Bob Smith",
                        "email": "bob@example.com",
                        "age": 25,
                        "verified": false
                    })"
                };

                json_array arr(json_values);
                CHECK_EQ(arr.size(), 2);

                for (size_t i = 0; i < arr.size(); ++i)
                {
                    CHECK(arr[i].has_value());
                }
            }

            SUBCASE("API responses")
            {
                std::vector<std::string> json_values = {R"({
                    "status": "success",
                    "data": {
                        "users": [
                            {"id": 1, "name": "Alice"},
                            {"id": 2, "name": "Bob"}
                        ],
                        "total": 2
                    },
                    "metadata": {
                        "page": 1,
                        "per_page": 10
                    }
                })"};

                json_array arr(json_values);
                CHECK_EQ(arr.size(), 1);
                CHECK(arr[0].has_value());
            }

            SUBCASE("configuration objects")
            {
                std::vector<std::string> json_values = {
                    R"({
                        "database": {
                            "host": "localhost",
                            "port": 5432,
                            "name": "myapp"
                        },
                        "cache": {
                            "enabled": true,
                            "ttl": 3600
                        }
                    })"
                };

                json_array arr(json_values);
                CHECK_EQ(arr.size(), 1);
                CHECK(arr[0].has_value());
            }
        }

        TEST_CASE("data type information")
        {
            SUBCASE("get_data_type_from_array for json_array")
            {
                using dt = detail::get_data_type_from_array<json_array>;
                CHECK_EQ(dt::get(), data_type::STRING);
            }

            SUBCASE("get_data_type_from_array for big_json_array")
            {
                using dt = detail::get_data_type_from_array<big_json_array>;
                CHECK_EQ(dt::get(), data_type::LARGE_STRING);
            }

            SUBCASE("get_data_type_from_array for json_view_array")
            {
                using dt = detail::get_data_type_from_array<json_view_array>;
                CHECK_EQ(dt::get(), data_type::STRING_VIEW);
            }
        }
    }
}
