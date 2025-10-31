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

#include <sparrow/c_interface.hpp>
#include <queue>
#include <ranges>


namespace sparrow
{
    class arrow_array_stream_private_data
    {
    public:
        arrow_array_stream_private_data() = default;

        void import_schema(ArrowSchema* out_schema)
        {
            if (m_schema != nullptr)
            {
                if (m_schema->release != nullptr)
                {
                    m_schema->release(m_schema);
                }
                delete m_schema;
                m_schema = nullptr;
            }

            m_schema = out_schema;
        }

        [[nodiscard]] const ArrowSchema& schema() const
        {
            return *m_schema;
        }

        template<std::ranges::input_range R>
        requires std::same_as<std::ranges::range_value_t<R>, ArrowArray*>
        void import_arrays(R&& arrays)
        {
            for (auto&& array : arrays)
            {
                m_arrays.push(array);
            }
        }

        void import_array(ArrowArray* array)
        {
            m_arrays.push(array);
        }

        ArrowArray* export_next_array()
        {
            if(m_arrays.empty())
            {
                return new ArrowArray{};
            }

            ArrowArray* array = m_arrays.front();
            m_arrays.pop();
            return array;
        }

        [[nodiscard]] const std::string& get_last_error_message() const
        {
            return m_last_error_message;
        }

    private:
        ArrowSchema* m_schema = nullptr;
        std::queue<ArrowArray*> m_arrays{};
        std::string m_last_error_message{};
    };
}
