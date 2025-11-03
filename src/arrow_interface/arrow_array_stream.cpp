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

#include "sparrow/arrow_interface/arrow_array_stream.hpp"

#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"

namespace sparrow
{
    void release_arrow_array_stream(ArrowArrayStream* stream)
    {
        SPARROW_ASSERT_FALSE(stream == nullptr);
        SPARROW_ASSERT_TRUE(stream->release == std::addressof(release_arrow_array_stream));
        if (stream->private_data != nullptr)
        {
            auto private_data = static_cast<arrow_array_stream_private_data*>(stream->private_data);
            delete private_data;
            stream->private_data = nullptr;
        }
        *stream = {};
    }

    int get_schema_from_arrow_array_stream(ArrowArrayStream* stream, ArrowSchema* out)
    {
        SPARROW_ASSERT_FALSE(stream == nullptr);
        SPARROW_ASSERT_FALSE(out == nullptr);

        if (stream->private_data == nullptr)
        {
            return EINVAL;
        }

        if (stream->release != std::addressof(release_arrow_array_stream))
        {
            return EINVAL;
        }

        try
        {
            auto private_data = static_cast<arrow_array_stream_private_data*>(stream->private_data);
            copy_schema(*private_data->schema(), *out);
            return 0;
        }
        catch (const std::bad_alloc&)
        {
            return ENOMEM;
        }
        catch (...)
        {
            return EIO;
        }
    }

    int get_next_from_arrow_array_stream(ArrowArrayStream* stream, ArrowArray* out)
    {
        SPARROW_ASSERT_FALSE(stream == nullptr);
        SPARROW_ASSERT_FALSE(out == nullptr);

        if (stream->private_data == nullptr)
        {
            return EINVAL;
        }

        if (stream->release != std::addressof(release_arrow_array_stream))
        {
            return EINVAL;
        }

        auto private_data = static_cast<arrow_array_stream_private_data*>(stream->private_data);

        try
        {
            ArrowArray* array_ptr = private_data->export_next_array();
            if (array_ptr->release == nullptr)
            {
                // End of stream - return empty ArrowArray
                *out = *array_ptr;
                delete array_ptr;
            }
            else
            {
                // Move array content to out
                *out = move_array(*array_ptr);
                delete array_ptr;
            }
            return 0;
        }
        catch (const std::bad_alloc&)
        {
            private_data->set_last_error_message("Memory allocation failed");
            return ENOMEM;
        }
        catch (...)
        {
            private_data->set_last_error_message("Unknown error occurred");
            return EIO;
        }
    }

    const char* get_last_error_from_arrow_array_stream(ArrowArrayStream* stream)
    {
        SPARROW_ASSERT_FALSE(stream == nullptr);

        if (stream->private_data == nullptr)
        {
            return nullptr;
        }

        if (stream->release == nullptr)
        {
            return nullptr;
        }

        if (stream->release != std::addressof(release_arrow_array_stream))
        {
            return nullptr;
        }

        auto private_data = static_cast<arrow_array_stream_private_data*>(stream->private_data);
        const std::string& error_msg = private_data->get_last_error_message();

        if (error_msg.empty())
        {
            return nullptr;
        }

        return error_msg.c_str();
    }

    void fill_arrow_array_stream(ArrowArrayStream& stream)
    {
        stream.get_last_error = &get_last_error_from_arrow_array_stream;
        stream.get_next = &get_next_from_arrow_array_stream;
        stream.get_schema = &get_schema_from_arrow_array_stream;
        stream.release = &release_arrow_array_stream;
        stream.private_data = new arrow_array_stream_private_data();
    }

    ArrowArrayStream make_empty_arrow_array_stream()
    {
        ArrowArrayStream stream{};
        fill_arrow_array_stream(stream);
        return stream;
    }
}