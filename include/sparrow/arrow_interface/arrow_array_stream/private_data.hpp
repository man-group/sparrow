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

#include <memory>
#include <queue>
#include <ranges>
#include <string>

#include <sparrow/c_interface.hpp>

namespace sparrow
{
    class arrow_array_stream_private_data
    {
    public:

        arrow_array_stream_private_data() = default;

        void import_schema(ArrowSchema* out_schema)
        {
            m_schema.reset(out_schema);
        }

        [[nodiscard]] ArrowSchema* schema()
        {
            return m_schema.get();
        }

        [[nodiscard]] const ArrowSchema* schema() const
        {
            return m_schema.get();
        }

        template <std::ranges::input_range R>
            requires std::same_as<std::ranges::range_value_t<R>, ArrowArray*>
        void import_arrays(R&& arrays)
        {
            for (auto&& array : arrays)
            {
                m_arrays.push(array_ptr(array));
            }
        }

        void import_array(ArrowArray* array)
        {
            m_arrays.push(array_ptr(array));
        }

        [[nodiscard]] ArrowArray* export_next_array()
        {
            if (m_arrays.empty())
            {
                return new ArrowArray{};
            }

            ArrowArray* array = m_arrays.front().release();
            m_arrays.pop();
            return array;
        }

        [[nodiscard]] const std::string& get_last_error_message() const
        {
            return m_last_error_message;
        }

        void set_last_error_message(std::string_view message)
        {
            m_last_error_message = message;
        }

    private:

        struct arrow_schema_deleter
        {
            void operator()(ArrowSchema* schema) const
            {
                if (schema != nullptr)
                {
                    if (schema->release != nullptr)
                    {
                        schema->release(schema);
                    }
                    delete schema;
                }
            }
        };

        struct arrow_array_deleter
        {
            void operator()(ArrowArray* array) const
            {
                if (array != nullptr)
                {
                    if (array->release != nullptr)
                    {
                        array->release(array);
                    }
                    delete array;
                }
            }
        };

        using schema_ptr = std::unique_ptr<ArrowSchema, arrow_schema_deleter>;
        using array_ptr = std::unique_ptr<ArrowArray, arrow_array_deleter>;

        schema_ptr m_schema;
        std::queue<array_ptr> m_arrays{};
        std::string m_last_error_message{};
    };
}
