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
#include "sparrow/arrow_interface/arrow_array_stream/private_data.hpp"
#include "sparrow/c_stream_interface.hpp"
#include "sparrow/layout/layout_concept.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    void release_arrow_array_stream(ArrowArrayStream* stream);

    class arrow_array_stream_proxy
    {
    public:

        arrow_array_stream_proxy()
            : m_stream_ptr(new ArrowArrayStream)
        {
        }

        explicit arrow_array_stream_proxy(ArrowArrayStream* stream_ptr)
            : m_stream_ptr(stream_ptr)
        {
            SPARROW_ASSERT_FALSE(stream_ptr == nullptr);
            SPARROW_ASSERT_TRUE(
                stream_ptr->release == nullptr
                || stream_ptr->release == std::addressof(release_arrow_array_stream)
            );
        }

        ~arrow_array_stream_proxy()
        {
            if (m_stream_ptr != nullptr && m_stream_ptr->release != nullptr)
            {
                m_stream_ptr->release(m_stream_ptr);
                m_stream_ptr = nullptr;
                delete m_stream_ptr;
            }
        }

        [[nodiscard]] const arrow_array_stream_private_data* get_private_data() const
        {
            SPARROW_ASSERT_FALSE(m_stream_ptr == nullptr);
            return static_cast<const arrow_array_stream_private_data*>(m_stream_ptr->private_data);
        }

        [[nodiscard]] arrow_array_stream_private_data* get_private_data()
        {
            SPARROW_ASSERT_FALSE(m_stream_ptr == nullptr);
            return static_cast<arrow_array_stream_private_data*>(m_stream_ptr->private_data);
        }

        [[nodiscard]] const ArrowArrayStream& stream() const
        {
            return *m_stream_ptr;
        }

        [[nodiscard]] ArrowArrayStream& stream()
        {
            return *m_stream_ptr;
        }

        template <layout A>
        void add_array(A&& array)
        {
            

                get_private_data()->import_array(ArrowArray *array)
        
        }

    private:

        [[nodiscard]] bool check_compatible_schema(const ArrowSchema& schema) const
        {
            throw_if_immutable();
            const auto* existing_schema = &get_private_data()->schema();
            return compare_arrow_schemas(*existing_schema, schema);

        }
        

        void throw_if_immutable() const
        {
             SPARROW_ASSERT_TRUE(m_stream_ptr != nullptr);
            if (m_stream_ptr->release == nullptr)
            {
                throw std::runtime_error("Cannot add array to released ArrowArrayStream");
            }
            if (m_stream_ptr->release != std::addressof(release_arrow_array_stream))
            {
                throw std::runtime_error("ArrowArrayStream release function is not valid");
            }
            if (m_stream_ptr->private_data == nullptr)
            {
                throw std::runtime_error("ArrowArrayStream private data is not initialized");
            }
        }

        ArrowArrayStream* m_stream_ptr;
    };
}