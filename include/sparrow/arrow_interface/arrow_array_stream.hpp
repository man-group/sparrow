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

/**
 * @file arrow_array_stream.hpp
 * @brief Implementation of the Arrow C Stream Interface for streaming data exchange.
 *
 * This file provides a complete implementation of the Arrow C Stream Interface as specified at:
 * https://arrow.apache.org/docs/format/CStreamInterface.html
 *
 * The Arrow C Stream Interface defines a standard C API for streaming Arrow data between different
 * libraries and components within a single process. It uses a pull-based iteration model where
 * consumers request data chunks one at a time.
 *
 * Key components:
 * - ArrowArrayStream: C structure with callbacks for stream operations (defined in c_stream_interface.hpp)
 * - arrow_array_stream_proxy: C++ wrapper providing RAII and type-safe operations
 *
 * @see https://arrow.apache.org/docs/format/CStreamInterface.html
 * @see https://arrow.apache.org/docs/format/CDataInterface.html
 */

#pragma once

#include "sparrow/arrow_interface/arrow_array_stream/private_data.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/c_stream_interface.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/utils/contracts.hpp"

namespace sparrow
{
    /**
     * @brief Release callback for ArrowArrayStream.
     *
     * This function implements the mandatory release callback as specified by the Arrow C Stream
     * Interface. It is responsible for cleaning up all resources associated with the stream,
     * including the private data.
     *
     * @param stream Pointer to the ArrowArrayStream to release. Must not be nullptr.
     *
     * @note After this function returns, all pointers in the stream structure are set to nullptr
     *       or zero, marking the stream as released.
     *
     * @see https://arrow.apache.org/docs/format/CStreamInterface.html
     */
    SPARROW_API void release_arrow_array_stream(ArrowArrayStream* stream);

    /**
     * @brief Get schema callback for ArrowArrayStream.
     *
     * This function implements the mandatory get_schema callback as specified by the Arrow C
     * Stream Interface. It allows the consumer to query the schema of the chunks of data in the
     * stream. The schema is the same for all data chunks.
     *
     * According to the specification:
     * - This callback is mandatory
     * - Must NOT be called on a released ArrowArrayStream
     * - Returns 0 on success, a non-zero error code otherwise
     * - The returned schema must be released independently by the consumer
     *
     * @param stream Pointer to the ArrowArrayStream.
     * @param out Pointer to an ArrowSchema structure to populate with the stream's schema.
     *
     * @return 0 on success, errno-compatible error code on failure (e.g., EINVAL, ENOMEM, EIO).
     *
     * @see https://arrow.apache.org/docs/format/CStreamInterface.html
     */
    SPARROW_API int get_schema_from_arrow_array_stream(ArrowArrayStream* stream, ArrowSchema* out);

    /**
     * @brief Get next array callback for ArrowArrayStream.
     *
     * This function implements the mandatory get_next callback as specified by the Arrow C Stream
     * Interface. It allows the consumer to get the next chunk of data in the stream.
     *
     * According to the specification:
     * - This callback is mandatory
     * - Must NOT be called on a released ArrowArrayStream
     * - Returns 0 on success, a non-zero error code otherwise
     * - On success, the consumer must check if the ArrowArray is released
     * - If the ArrowArray is released, the end of stream has been reached
     * - Otherwise, the ArrowArray contains a valid data chunk
     * - The returned array must be released independently by the consumer
     *
     * @param stream Pointer to the ArrowArrayStream.
     * @param out Pointer to an ArrowArray structure to populate with the next data chunk.
     *            Will be a released array (release == nullptr) when the end of stream is reached.
     *
     * @return 0 on success, errno-compatible error code on failure (e.g., EINVAL, ENOMEM, EIO).
     *
     * @see https://arrow.apache.org/docs/format/CStreamInterface.html
     */
    SPARROW_API int get_next_from_arrow_array_stream(ArrowArrayStream* stream, ArrowArray* out);

    /**
     * @brief Get last error callback for ArrowArrayStream.
     *
     * This function implements the mandatory get_last_error callback as specified by the Arrow C
     * Stream Interface. It allows the consumer to get a textual description of the last error.
     *
     * According to the specification:
     * - This callback is mandatory
     * - Must ONLY be called if the last operation returned an error
     * - Must NOT be called on a released ArrowArrayStream
     * - Returns a pointer to a NULL-terminated UTF8-encoded string, or NULL if no detailed
     *   description is available
     * - The returned pointer is only guaranteed to be valid until the next callback call
     *
     * @param stream Pointer to the ArrowArrayStream.
     *
     * @return Pointer to a NULL-terminated error message string, or NULL if unavailable.
     *
     * @see https://arrow.apache.org/docs/format/CStreamInterface.html
     */
    SPARROW_API const char* get_last_error_from_arrow_array_stream(ArrowArrayStream* stream);

    SPARROW_API void fill_arrow_array_stream(ArrowArrayStream& stream);

    /**
     * @brief Move an ArrowArrayStream by transferring ownership of its resources.
     *
     * Performs a shallow copy of the stream structure and zeros out the source.
     * After this operation, the source stream is left in a released-like state
     * with all function pointers and data pointers set to nullptr or zero.
     *
     * This should be used when you need to transfer ownership of the stream's resources,
     * rather than copying. Regular assignment does not transfer ownership and may lead to
     * double-free or resource leaks.
     *
     * @param source The source stream to move from (rvalue reference).
     *               Must not be released or nullptr.
     * @return A new ArrowArrayStream with ownership of the source's resources.
     *
     * @post source will have all pointers set to nullptr/null/zero.
     *
     * @see https://arrow.apache.org/docs/format/CStreamInterface.html
     */
    SPARROW_API ArrowArrayStream move_array_stream(ArrowArrayStream&& source);

    /**
     * @brief Move an ArrowArrayStream by transferring ownership of its resources.
     *
     * Performs a shallow copy of the stream structure and zeros out the source.
     * After this operation, the source stream is left in a released-like state
     * with all function pointers and data pointers set to nullptr or zero.
     *
     * This should be used when you need to transfer ownership of the stream's resources,
     * rather than copying. Regular assignment does not transfer ownership and may lead to
     * double-free or resource leaks.
     *
     * @param source The source stream to move from (lvalue reference).
     *               Must not be released or nullptr.
     * @return A new ArrowArrayStream with ownership of the source's resources.
     *
     * @post source will have all pointers set to nullptr/null/zero.
     *
     * @see https://arrow.apache.org/docs/format/CStreamInterface.html
     */
    SPARROW_API ArrowArrayStream move_array_stream(ArrowArrayStream& source);
}
