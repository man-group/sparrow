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

#include <ranges>

#include "sparrow/array.hpp"
#include "sparrow/array_api.hpp"
#include "sparrow/arrow_interface/arrow_array.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/arrow_interface/arrow_array_stream.hpp"
#include "sparrow/arrow_interface/arrow_schema.hpp"
#include "sparrow/c_interface.hpp"
#include "sparrow/layout/layout_concept.hpp"

namespace sparrow
{
    /**
     * @brief C++ proxy class for managing ArrowArrayStream objects.
     *
     * This class provides a C++ interface for working with ArrowArrayStream objects as defined
     * by the Arrow C Stream Interface specification. It manages the lifetime of the stream and
     * provides convenient methods for adding and retrieving data chunks.
     *
     * The Arrow C Stream Interface is designed for streaming data exchange between different
     * libraries or components within a single process. A stream exposes a source of data chunks,
     * where each chunk has the same schema.
     *
     * Key features:
     * - Automatic resource management through RAII
     * - Type-safe push/pop operations for array data
     * - Schema validation when adding arrays
     * - Proper implementation of all mandatory ArrowArrayStream callbacks
     *
     * Thread safety:
     * - The stream is not thread-safe by design (per specification)
     * - Concurrent calls to get_next or pop must be externally synchronized
     *
     * @note This class implements the producer side of the Arrow C Stream Interface.
     *       It creates streams that can be consumed by other libraries that understand
     *       the ArrowArrayStream C interface.
     *
     * @see https://arrow.apache.org/docs/format/CStreamInterface.html
     */
    class SPARROW_API arrow_array_stream_proxy
    {
    public:

        /**
         * @brief Constructs a new ArrowArrayStream producer.
         *
         * Creates a new stream with an empty queue of arrays. The stream must be populated
         * with a schema and arrays before it can be consumed.
         *
         * @note The created stream has all callbacks properly initialized according to the
         *       Arrow C Stream Interface specification.
         */
        arrow_array_stream_proxy();

        /**
         * @brief Wraps an existing ArrowArrayStream pointer.
         *
         * Takes ownership of an externally created ArrowArrayStream. The stream must either be
         * unreleased (release == nullptr) or have been created by this library
         * (release == release_arrow_array_stream).
         *
         * @param stream_ptr Pointer to the ArrowArrayStream to wrap. Must not be nullptr.
         *
         * @throws Assertion failure if stream_ptr is nullptr or has an incompatible release callback.
         */
        explicit arrow_array_stream_proxy(ArrowArrayStream* stream_ptr);

        // explicit arrow_array_stream_proxy(ArrowSchema* schema_ptr);

        arrow_array_stream_proxy(const arrow_array_stream_proxy&) = delete;
        arrow_array_stream_proxy& operator=(const arrow_array_stream_proxy&) = delete;

        /**
         * @brief Destructor that releases all resources.
         *
         * Calls the release callback on both the schema and stream if they are not already
         * released. This ensures proper cleanup of all Arrow C interface objects.
         */
        ~arrow_array_stream_proxy();

        /**
         * @brief Export ownership of the stream pointer.
         *
         * Transfers ownership of the stream to the caller. After this call, the proxy no longer
         * manages the stream's lifetime, and the caller is responsible for calling the release
         * callback when done.
         *
         * This is useful for passing the stream to external C APIs that will take ownership.
         *
         * @return Pointer to the ArrowArrayStream. The caller must eventually call release.
         */
        ArrowArrayStream* export_stream();

        /**
         * @brief Adds a range of arrays to the stream.
         *
         * Pushes multiple arrays into the stream's queue. All arrays must have schemas compatible
         * with the stream's schema. Arrays are validated before being added.
         *
         * @tparam R A range type whose value type satisfies the layout concept.
         * @param arrays The range of arrays to add to the stream.
         *
         * @throws std::runtime_error If any array has an incompatible schema.
         * @throws std::runtime_error If the stream is immutable (released or not initialized).
         */
        template <std::ranges::input_range R>
            requires layout<std::ranges::range_value_t<R>>
        void push(R&& arrays)
        {
            arrow_array_stream_private_data& private_data = *get_private_data();
            
            // Check if we need to create schema from first array
            if (private_data.schema() == nullptr)
            {
                ArrowSchema* schema = new ArrowSchema();
                copy_schema(*get_arrow_schema(*std::ranges::begin(arrays)), *schema);
                private_data.import_schema(schema);
            }
            
            // Validate schema compatibility for all arrays
            for (const auto& array : arrays)
            {
                if (!check_compatible_schema(*private_data.schema(), *get_arrow_schema(array)))
                {
                    throw std::runtime_error("Incompatible schema when adding array to ArrowArrayStream");
                }
            }
            if (private_data.schema() == nullptr)
            {
                ArrowSchema* schema = new ArrowSchema();
                swap(*schema, *get_arrow_schema(*std::ranges::begin(arrays)));
                private_data.import_schema(schema);
            }

            // Import all arrays
            for (auto&& array : arrays)
            {
                ArrowArray extracted_array = extract_arrow_array(std::move(array));
                ArrowArray* arrow_array_ptr = new ArrowArray();
                swap(*arrow_array_ptr, extracted_array);
                private_data.import_array(arrow_array_ptr);
            }
        }

        /**
         * @brief Adds a single array to the stream.
         *
         * Pushes a single array into the stream's queue. The array must have a schema compatible
         * with the stream's schema.
         *
         * @tparam A A type satisfying the layout concept.
         * @param array The array to add to the stream.
         *
         * @throws std::runtime_error If the array has an incompatible schema.
         * @throws std::runtime_error If the stream is immutable (released or not initialized).
         */
        template <layout A>
        void push(A&& array)
        {
            std::ranges::single_view<std::decay_t<A>> view(std::forward<A>(array));
            push(view);
        }

        /**
         * @brief Retrieves the next array from the stream.
         *
         * Removes and returns the next array from the stream's queue. This implements the
         * consumer-side pop operation, similar to calling get_next on the ArrowArrayStream.
         *
         * @return The next array in the stream.
         *
         * @throws std::system_error If getting the schema fails.
         * @throws std::runtime_error If the stream is immutable.
         *
         * @note If the queue is empty, returns a released (empty) array, indicating end of stream.
         */
        std::optional<array> pop();

    private:

        ArrowArrayStream* m_stream_ptr;

        /**
         * @brief Validates that the stream is mutable.
         *
         * Checks that the stream has not been released and that it has valid callbacks and
         * private data. This must be called before any mutation operations.
         *
         * @throws std::runtime_error If the stream is released, has an invalid release callback,
         *                            or has uninitialized private data.
         */
        void throw_if_immutable() const;

                /**
         * @brief Gets the private data (const version).
         *
         * @return Const pointer to the stream's private data.
         */
        [[nodiscard]] const arrow_array_stream_private_data* get_private_data() const;

        /**
         * @brief Gets the private data (mutable version).
         *
         * @return Mutable pointer to the stream's private data.
         */
        [[nodiscard]] arrow_array_stream_private_data* get_private_data();
    };
}