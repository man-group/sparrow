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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <functional>

namespace sparrow
{
    /// Hash function object for std::reference_wrapper.
    ///
    /// This function object provides a hash function for std::reference_wrapper.
    /// It computes the hash value of the referenced object using std::hash,
    /// and returns the result.
    ///
    /// @tparam T The type of the referenced object.
    struct reference_wrapper_hasher
    {
        template <typename T>
        std::size_t operator()(const std::reference_wrapper<T>& ref) const
        {
            return std::hash<std::remove_cvref_t<T>>{}(ref.get());
        }
    };

    /// Functor for comparing two reference wrappers.
    ///
    /// This functor is used to compare two reference wrappers for equality.
    /// It compares the underlying referenced objects using the `==` operator.
    ///
    /// @tparam T The type of the referenced object.
    struct reference_wrapper_equal
    {
        template <typename T>
        bool operator()(const std::reference_wrapper<T>& lhs, const std::reference_wrapper<T>& rhs) const
        {
            return lhs.get() == rhs.get();
        }
    };

    namespace mpl
    {
        template <typename T>
        constexpr bool is_reference_wrapper_v = false;

        template <typename U>
        constexpr bool is_reference_wrapper_v<std::reference_wrapper<U>> = true;

        /// Checks if the given type is a reference wrapper.
        ///
        /// @tparam T The type to check.
        /// @param  The instance of the type to check.
        /// @return `true` if the type is a reference wrapper, `false` otherwise.
        template <typename T>
        constexpr bool is_reference_wrapper(const T&)
        {
            return is_reference_wrapper_v<std::remove_cvref_t<T>>;
        }
    }
}  // namespace sparrow
