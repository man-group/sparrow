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
    class arrow_array_stream_proxy
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
        SPARROW_API arrow_array_stream_proxy();

        /**
         * @brief Constructs from an existing ArrowArrayStream by taking ownership.
         *
         * Moves an externally created ArrowArrayStream into this proxy. This allows
         * stack-allocated or otherwise-owned ArrowArrayStream objects to be transferred
         * into the proxy without additional heap allocation.
         *
         * @param stream The ArrowArrayStream to move and take ownership of.
         *
         * @post This proxy owns the stream and will release it on destruction
         */
        SPARROW_API explicit arrow_array_stream_proxy(ArrowArrayStream&& stream);

        /**
         * @brief Constructs from an existing ArrowArrayStream pointer by referencing it.
         *
         * References an externally created ArrowArrayStream without taking ownership.
         * The stream must remain valid for the lifetime of this proxy.
         *
         * @param stream Pointer to the ArrowArrayStream to reference (not owned).
         *
         * @pre stream must be a valid pointer to ArrowArrayStream
         * @pre External stream must remain valid for the lifetime of this proxy
         * @post This proxy does not own the stream
         * @post External stream must be managed by the caller
         */
        SPARROW_API explicit arrow_array_stream_proxy(ArrowArrayStream* stream);

        // explicit arrow_array_stream_proxy(ArrowSchema* schema_ptr);

        arrow_array_stream_proxy(const arrow_array_stream_proxy&) = delete;
        arrow_array_stream_proxy& operator=(const arrow_array_stream_proxy&) = delete;

        SPARROW_API
        arrow_array_stream_proxy(arrow_array_stream_proxy&& other) noexcept;
        SPARROW_API
        arrow_array_stream_proxy& operator=(arrow_array_stream_proxy&& other) noexcept;

        /**
         * @brief Destructor that releases all resources.
         *
         * Calls the release callback on both the schema and stream if they are not already
         * released. This ensures proper cleanup of all Arrow C interface objects.
         */
        SPARROW_API ~arrow_array_stream_proxy();

        /**
         * Check whether the proxy has ownership of its internal `ArrowArrayStream`.
         */
        [[nodiscard]] SPARROW_API bool owns_stream() const;

        /**
         * @brief Export the stream pointer.
         *
         * Returns a pointer to the stream. If this proxy owns the stream, ownership is
         * transferred. If this proxy references an external stream, a pointer to that
         * stream is returned.
         *
         * This is useful for passing the stream to external C APIs that will take ownership.
         *
         * @return Pointer to the ArrowArrayStream.
         *
         * @post If stream was owned, this proxy is left in a released state
         * @post If stream was referenced, pointer to external stream is returned
         */
        [[nodiscard]] SPARROW_API ArrowArrayStream* export_stream();

        /**
         * @brief Adds a range of arrays to the stream.
         *
         * Pushes multiple arrays into the stream's queue. All arrays must have schemas compatible
         * with the stream's schema. Arrays are validated before being added.
         *
         * @tparam R A range type whose value type satisfies the layout_or_array concept.
         * @param arrays The range of arrays to add to the stream.
         *
         * @throws std::runtime_error If any array has an incompatible schema.
         * @throws std::runtime_error If the stream is immutable (released or not initialized).
         */
        template <std::ranges::input_range R>
            requires layout_or_array<std::ranges::range_value_t<R>>
        void push(R&& arrays)
        {
            if(std::ranges::empty(arrays))
            {
                return;
            }
            
            arrow_array_stream_private_data& private_data = get_private_data();

            // Check if we need to create schema from first array
            if (private_data.schema() == nullptr)
            {
                schema_unique_ptr schema{new ArrowSchema(), arrow_schema_deleter{}};
                copy_schema(*get_arrow_schema(*std::ranges::begin(arrays)), *schema);
                private_data.import_schema(std::move(schema));
            }

            // Validate schema compatibility for all arrays
            for (const auto& array : arrays)
            {
                if (!check_compatible_schema(*private_data.schema(), *get_arrow_schema(array)))
                {
                    throw std::runtime_error("Incompatible schema when adding array to ArrowArrayStream");
                }
            }

            // Import all arrays
            for (auto&& array : std::forward<R>(arrays))
            {
                ArrowArray extracted_array = extract_arrow_array(std::move(array));
                array_unique_ptr array_ptr{new ArrowArray(), arrow_array_deleter{}};
                swap(*array_ptr, extracted_array);
                private_data.import_array(std::move(array_ptr));
            }
        }

        /**
         * @brief Adds a single array to the stream.
         *
         * Pushes a single array into the stream's queue. The array must have a schema compatible
         * with the stream's schema.
         *
         * @tparam A A type satisfying the layout_or_array concept.
         * @param array The array to add to the stream.
         *
         * @throws std::runtime_error If the array has an incompatible schema.
         * @throws std::runtime_error If the stream is immutable (released or not initialized).
         */
        template <layout_or_array A>
        void push(A&& array)
        {
            push(std::ranges::single_view(std::forward<A>(array)));
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
        SPARROW_API std::optional<array> pop();

    private:

        std::variant<ArrowArrayStream*, ArrowArrayStream> m_stream;

        /**
         * @brief Gets the stream pointer from variant.
         *
         * @return Mutable pointer to the stream.
         */
        [[nodiscard]] ArrowArrayStream* get_stream_ptr();

        /**
         * @brief Gets the stream pointer from variant (const version).
         *
         * @return Const pointer to the stream.
         */
        [[nodiscard]] const ArrowArrayStream* get_stream_ptr() const;

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
         * @brief Gets the private data (mutable version).
         *
         * @return Reference to the stream's private data.
         */
        [[nodiscard]] SPARROW_API arrow_array_stream_private_data& get_private_data();
    };
}
