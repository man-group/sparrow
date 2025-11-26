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
        : m_stream(ArrowArrayStream{})
    {
        fill_arrow_array_stream(std::get<ArrowArrayStream>(m_stream));
    }

    arrow_array_stream_proxy::arrow_array_stream_proxy(ArrowArrayStream&& stream)
        : m_stream(move_array_stream(stream))
    {
    }

    arrow_array_stream_proxy::arrow_array_stream_proxy(ArrowArrayStream* stream)
        : m_stream(stream)
    {
        SPARROW_ASSERT_FALSE(stream == nullptr);
    }

    arrow_array_stream_proxy::arrow_array_stream_proxy(arrow_array_stream_proxy&& other) noexcept
        : m_stream(other.m_stream)
    {
        if (std::holds_alternative<ArrowArrayStream>(other.m_stream))
        {
            other.m_stream = ArrowArrayStream{};
        }
        else
        {
            other.m_stream = static_cast<ArrowArrayStream*>(nullptr);
        }
    }

    arrow_array_stream_proxy& arrow_array_stream_proxy::operator=(arrow_array_stream_proxy&& other) noexcept
    {
        if (this != &other)
        {
            // Release current stream if owned
            if (std::holds_alternative<ArrowArrayStream>(m_stream))
            {
                auto& stream = std::get<ArrowArrayStream>(m_stream);
                if (stream.release != nullptr)
                {
                    stream.release(&stream);
                }
            }

            m_stream = other.m_stream;

            if (std::holds_alternative<ArrowArrayStream>(other.m_stream))
            {
                other.m_stream = ArrowArrayStream{};
            }
            else
            {
                other.m_stream = static_cast<ArrowArrayStream*>(nullptr);
            }
        }
        return *this;
    }

    arrow_array_stream_proxy::~arrow_array_stream_proxy()
    {
        if (std::holds_alternative<ArrowArrayStream>(m_stream))
        {
            auto& stream = std::get<ArrowArrayStream>(m_stream);
            if (stream.release != nullptr)
            {
                stream.release(&stream);
            }
        }
    }

    ArrowArrayStream* arrow_array_stream_proxy::get_stream_ptr()
    {
        if (std::holds_alternative<ArrowArrayStream*>(m_stream))
        {
            return std::get<ArrowArrayStream*>(m_stream);
        }
        else
        {
            return &std::get<ArrowArrayStream>(m_stream);
        }
    }

    const ArrowArrayStream* arrow_array_stream_proxy::get_stream_ptr() const
    {
        if (std::holds_alternative<ArrowArrayStream*>(m_stream))
        {
            return std::get<ArrowArrayStream*>(m_stream);
        }
        else
        {
            return &std::get<ArrowArrayStream>(m_stream);
        }
    }

    arrow_array_stream_private_data& arrow_array_stream_proxy::get_private_data()
    {
        throw_if_immutable();
        return *static_cast<arrow_array_stream_private_data*>(get_stream_ptr()->private_data);
    }

    ArrowArrayStream* arrow_array_stream_proxy::export_stream()
    {
        if (std::holds_alternative<ArrowArrayStream*>(m_stream))
        {
            // Return pointer to external stream (no ownership transfer)
            return std::get<ArrowArrayStream*>(m_stream);
        }
        else
        {
            // Transfer ownership by returning pointer to our owned stream
            // We need to keep the stream alive, so we allocate and move it
            ArrowArrayStream* exported = new ArrowArrayStream(std::get<ArrowArrayStream>(m_stream));
            m_stream = ArrowArrayStream{};
            return exported;
        }
    }

    std::optional<array> arrow_array_stream_proxy::pop()
    {
        ArrowArrayStream* stream = get_stream_ptr();
        ArrowArray array{};
        if (int err = stream->get_next(stream, &array); err != 0)
        {
            throw std::system_error(
                err,
                std::generic_category(),
                "Failed to get next array from ArrowArrayStream: " + std::string(stream->get_last_error(stream))
            );
        }

        if (array.release == nullptr)
        {
            // End of stream
            return std::nullopt;
        }

        ArrowSchema schema{};
        if (int err = stream->get_schema(stream, &schema); err != 0)
        {
            throw std::system_error(err, std::generic_category(), "Failed to get schema from ArrowArrayStream");
        }

        return sparrow::array(std::move(array), std::move(schema));
    }

    void arrow_array_stream_proxy::throw_if_immutable() const
    {
        const ArrowArrayStream* stream = get_stream_ptr();
        if (stream == nullptr)
        {
            throw std::runtime_error("ArrowArrayStream pointer is null");
        }
        if (stream->release == nullptr)
        {
            throw std::runtime_error("Cannot add array to released ArrowArrayStream");
        }
        if (stream->release != std::addressof(release_arrow_array_stream))
        {
            throw std::runtime_error("ArrowArrayStream release function is not valid");
        }
        if (stream->private_data == nullptr)
        {
            throw std::runtime_error("ArrowArrayStream private data is not initialized");
        }
    }
}
