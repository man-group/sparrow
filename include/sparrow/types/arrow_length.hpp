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

#include <limits>
#include <cstdint>
#include <concepts>
#include <stdexcept>
#include <utility>
#include <string>
#include <typeindex>

#include "sparrow/config/config.hpp"

namespace sparrow
{

    /// Type representing an Arrow length (sizes, offsets) in the Arrow specification and storage.
    /// This is used internally but is not always directly convertible to `std::size_t`
    /// and `std::ptrdiff_t` which are the "native" types to represent sizes and offsets.
    /// For conversion purposes:
    /// - @see `to_native_size()`
    /// - @see `to_native_offset()`
    /// - @see `to_arrow_length()`
    /// - @see `sum_arrow_offsets()`
    using arrow_length = std::int64_t;

    /// Clarifies if a length value is supposed to be a size/length or an offset.
    /// Offsets can be negative, sizes cannot. This is only important for runtime checks
    /// and should only be used when calling functions that do runtime checks on sizes
    /// and offsets values validity.
    enum class arrow_length_kind
    {
        size, offset
    };

    /// Maximum size allowed for arrow lengths.
    /// This can be constrained to 32bit signed values using configuration options.
    /// See: https://arrow.apache.org/docs/format/Columnar.html#array-lengths
    static constexpr arrow_length max_arrow_length = config::enable_32bit_size_limit
                                                         ? std::numeric_limits<std::int32_t>::max()
                                                         : std::numeric_limits<arrow_length>::max();

    /// @returns True if the provided value is in a valid range for an arrow size.
    ///          By default the range is zero to `max_arrow_length`, but if it is specified
    ///          that the value is an offset, we allow negatives values too.
    template <std::integral T>
    inline constexpr
    bool is_valid_arrow_length(T size_or_offset, arrow_length_kind kind = arrow_length_kind::size) noexcept
    {
        return std::in_range<arrow_length>(size_or_offset)
           and (kind == arrow_length_kind::offset or size_or_offset >= T(0))
           and size_or_offset <= max_arrow_length
           ;
    }

    /// Throws `std::runtime_error` if the provided value is not in the
    /// valid range of arrow length values or if the value is not representable
    /// in the specified `TargetRepr` type.
    /// The checks is only enabled if `config::enable_size_limit_runtime_check == true`,
    /// otherwise this function is no-op.
    template<std::integral TargetRepr = arrow_length>
    inline constexpr
    void throw_if_invalid_size(std::integral auto size_or_offset,
                               arrow_length_kind kind = arrow_length_kind::size)
    {
        if constexpr (config::enable_size_limit_runtime_check)
        {
            // checks that the value is in range of arrow length in general
            if (not is_valid_arrow_length(size_or_offset, kind))
            {
                throw std::runtime_error(  // TODO: replace by sparrow-specific error
                    // TODO: use std::format instead once available
                    std::string("size/length/offset is outside the valid arrow length limits [0:")
                    + std::to_string(max_arrow_length) + "] : " + std::to_string(size_or_offset) + "("
                    + typeid(size_or_offset).name() + ") "
                );
            }

            if constexpr (not std::same_as<arrow_length, TargetRepr>) // Already checked in `is_valid_size()` otherwise
            {
                // check that the value is representable by the specified type TargetRepr
                if (not std::in_range<TargetRepr>(size_or_offset))
                {
                    throw std::runtime_error(  // TODO: replace by sparrow-specific error
                        // TODO: use std::format instead once available
                        std::string("size/length/offset cannot be represented by ") + typeid(TargetRepr).name()
                        + ": " + std::to_string(size_or_offset) + " (" + typeid(size_or_offset).name() + ")"
                    );
                }
            }
        }
    }


#if defined(__GNUC__) && !defined(__clang__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wuseless-cast" // We want to be able to cast type aliases to their real types.
#endif

    /// @returns The provided arrow length value as represented by the native standard size type `std::size_t`.
    ///          If `config::enable_size_limit_runtime_check == true` it will also check that the value is
    ///          a valid arrow length and representable by `std::size_t` or throws otherwise.
    /// @throws  @see `throw_if_invalid_size()` for details.
    inline constexpr
    auto to_native_size(arrow_length length) -> std::size_t
    {
        throw_if_invalid_size<std::size_t>(length);
        return static_cast<std::size_t>(length);
    }

    /// @returns The provided arrow length value as represented by the native standard offset type `std::ptrdiff_t`.
    ///          If `config::enable_size_limit_runtime_check == true` it will also check that the value is
    ///          a valid arrow length and representable by `std::ptrdiff_t` or throws otherwise.
    /// @throws  @see `throw_if_invalid_size()` for details.
    inline constexpr
    auto to_native_offset(arrow_length offset) -> std::ptrdiff_t
    {
        throw_if_invalid_size<std::ptrdiff_t>(offset, arrow_length_kind::offset);
        return static_cast<std::ptrdiff_t>(offset);
    }

    /// @returns The provided size or offset value as represented by an arrow-length type (int64_t).
    ///          If `config::enable_size_limit_runtime_check == true` it will also check that the value is
    ///          a valid arrow length and representable in `int64_t` or throws otherwise.
    /// @throws  @see `throw_if_invalid_size()` for details.
    inline constexpr
    auto
    to_arrow_length(std::integral auto size_or_offset, arrow_length_kind kind = arrow_length_kind::size)
        -> arrow_length
    {
        throw_if_invalid_size(size_or_offset, kind);
        return static_cast<arrow_length>(size_or_offset);
    }

    /// @returns The sum of the provided offsets with `R` representation, whatever the offset types as long as
    ///          they are integrals and can represent and arrow length.
    ///          If `config::enable_size_limit_runtime_check == true` it will also check that each value and the resulting sum are
    ///          valid arrow lengths and representable in the specified result `R` or throws otherwise.
    /// @throws  @see `throw_if_invalid_size()` for details.
    template<std::integral R = arrow_length>
    inline constexpr
    auto sum_arrow_offsets(std::integral auto... offsets) -> R
    {
        // We cast every offset as an arrow length, then sum them as arrow length representation,
        // and finally cast back to the expected result type after verifying that
        // the resulting value is still a valid arrow length.
        auto result = (to_arrow_length(offsets, arrow_length_kind::offset) + ...);
        throw_if_invalid_size<R>(result, arrow_length_kind::size); // dont allow negatives as the result must be a size
        return static_cast<R>(result);
    }
#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif

}