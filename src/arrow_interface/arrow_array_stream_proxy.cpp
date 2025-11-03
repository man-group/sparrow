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


#include "sparrow/arrow_interface/arrow_array_stream_proxy.hpp"
#include <optional>

namespace sparrow
{
    arrow_array_stream_proxy::arrow_array_stream_proxy()
        : m_stream_ptr(new ArrowArrayStream)
    {
        fill_arrow_array_stream(*m_stream_ptr);
    }

    arrow_array_stream_proxy::arrow_array_stream_proxy(ArrowArrayStream* stream_ptr)
        : m_stream_ptr(stream_ptr)
    {
        SPARROW_ASSERT_FALSE(stream_ptr == nullptr);
    }

    arrow_array_stream_proxy::~arrow_array_stream_proxy()
    {
        if (m_stream_ptr != nullptr && m_stream_ptr->release != nullptr)
        {
            m_stream_ptr->release(m_stream_ptr);
            m_stream_ptr = nullptr;
            delete m_stream_ptr;
        }
    }

    const arrow_array_stream_private_data* arrow_array_stream_proxy::get_private_data() const
    {
        throw_if_immutable();
        return static_cast<const arrow_array_stream_private_data*>(m_stream_ptr->private_data);
    }

    arrow_array_stream_private_data* arrow_array_stream_proxy::get_private_data()
    {
        throw_if_immutable();
        return static_cast<arrow_array_stream_private_data*>(m_stream_ptr->private_data);
    }

    ArrowArrayStream* arrow_array_stream_proxy::export_stream()
    {
        ArrowArrayStream* temp = m_stream_ptr;
        m_stream_ptr = nullptr;
        return temp;
    }

    std::optional<array> arrow_array_stream_proxy::pop()
    {
        ArrowSchema schema;
        if (int err = m_stream_ptr->get_schema(m_stream_ptr, &schema); err != 0)
        {
            throw std::system_error(err, std::generic_category(), "Failed to get schema from ArrowArrayStream");
        }
        
        ArrowArray array;
        if (int err = m_stream_ptr->get_next(m_stream_ptr, &array); err != 0)
        {
            schema.release(&schema);
            throw std::system_error(err, std::generic_category(), "Failed to get next array from ArrowArrayStream");
        }
        
        if(array.release == nullptr)
        {
            // End of stream
            schema.release(&schema);
            return std::nullopt;
        }
        
        return std::make_optional<sparrow::array>(std::move(array), std::move(schema));
    }

    void arrow_array_stream_proxy::throw_if_immutable() const
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


}