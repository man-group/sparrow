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
#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/utils/memory.hpp"

namespace sparrow
{
    class run_end_encoded_array;

    class run_end_encoded_reference : public array_traits::value_type
    {
    public:

        using base_type = array_traits::value_type;
        using value_type = array_traits::value_type;
        using const_reference = array_traits::const_reference;

        SPARROW_API run_end_encoded_reference(run_end_encoded_array& array, std::size_t index);
        SPARROW_API
        run_end_encoded_reference(run_end_encoded_array& array, std::size_t index, std::size_t run_index);

        run_end_encoded_reference(const run_end_encoded_reference&) = default;
        run_end_encoded_reference(run_end_encoded_reference&&) noexcept = default;
        run_end_encoded_reference& operator=(run_end_encoded_reference&&) noexcept = default;

        SPARROW_API run_end_encoded_reference& operator=(const run_end_encoded_reference& rhs);
        SPARROW_API run_end_encoded_reference& operator=(const const_reference& rhs);
        SPARROW_API run_end_encoded_reference& operator=(const value_type& rhs);

        [[nodiscard]] SPARROW_API operator const_reference() const;

    private:

        [[nodiscard]] SPARROW_API const_reference current_value() const;

        SPARROW_API void refresh();

        run_end_encoded_array* p_array = nullptr;
        std::size_t m_index = 0;
        mutable std::size_t m_run_index = 0;
    };
}

#include "sparrow/layout/run_end_encoded_iterator.hpp"

namespace sparrow
{

    namespace detail
    {
        template <>
        struct get_data_type_from_array<run_end_encoded_array>
        {
            [[nodiscard]] static constexpr sparrow::data_type get()
            {
                return sparrow::data_type::RUN_ENCODED;
            }
        };
    }

    /**
     * Checks whether T is a run_end_encoded_array type.
     */
    template <class T>
    constexpr bool is_run_end_encoded_array_v = std::same_as<T, run_end_encoded_array>;

    /**
     * A run-end encoded array.
     * To use for data with long runs of identical values
     *
     * This array is used to store data in a run-length encoded format, where each run is represented by a
     * length and a value. Compresses data by storing run lengths for consecutive identical values.
     *
     * Performance notes:
     * - Random access is $O(\log R)$ where $R$ is the number of encoded runs.
     * - Iterator increment is amortized $O(1)$ because iterators cache the current run.
     * - Mutating operations work on the encoded run representation rather than the full logical length.
     *   Inserting repeated copies of a single value and erasing a contiguous logical range both run in
     *   $O(R_t)$, where $R_t$ is the number of encoded runs at or after the splice point.
     * - Sliced arrays (non-zero offset) are read-only for mutation APIs.
     *
     * Related Apache Arrow description and specification:
     * - https://arrow.apache.org/docs/dev/format/Intro.html#run-end-encoded-layout
     * - https://arrow.apache.org/docs/format/Columnar.html#run-end-encoded-layout
     */
    class run_end_encoded_array
    {
    public:

        using self_type = run_end_encoded_array;
        using size_type = std::size_t;
        using inner_value_type = array_traits::inner_value_type;
        using value_type = array_traits::value_type;
        using reference = run_end_encoded_reference;
        using const_reference = array_traits::const_reference;
        using iterator = run_encoded_array_iterator<false>;
        using const_iterator = run_encoded_array_iterator<true>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /**
         * @brief Constructs run-end encoded array from Arrow proxy.
         *
         * @param proxy Arrow proxy containing run-end encoded array data and schema
         *
         * @pre proxy must contain valid run-end encoded array and schema
         * @pre proxy format must be "+r"
         * @pre proxy must have two children arrays
         * @post Array is initialized with data from proxy
         */
        SPARROW_API explicit run_end_encoded_array(arrow_proxy proxy);

        /**
         * @brief Generic constructor for creating run-end encoded array.
         *
         * Creates a run-end encoded array from various input types.
         * Arguments are forwarded to compatible create_proxy() functions.
         *
         * @tparam Args Parameter pack for constructor arguments
         * @param args Constructor arguments (data ranges, validity, metadata, etc.)
         *
         * @pre Arguments must match one of the create_proxy() overload signatures
         * @post Array is created the specified data and configuration.
         */
        template <class... Args>
            requires(mpl::excludes_copy_and_move_ctor_v<run_end_encoded_array, Args...>)
        explicit run_end_encoded_array(Args&&... args)
            : run_end_encoded_array(create_proxy(std::forward<Args>(args)...))
        {
        }

        /**
         * @brief Copy constructor.
         *
         * @param rhs Source array to copy from
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Child arrays and offset pointers are reconstructed
         */
        SPARROW_API run_end_encoded_array(const self_type&);

        /**
         * @brief Copy assignment operator.
         *
         * @param rhs Source array to copy from
         * @return Reference to this array
         *
         * @pre rhs must be in a valid state
         * @post This array contains a deep copy of rhs data
         * @post Previous data is properly released
         * @post Child arrays and offset pointers are reconstructed
         */
        SPARROW_API self_type& operator=(const self_type&);
        ~run_end_encoded_array() = default;

        run_end_encoded_array(self_type&&) = default;
        self_type& operator=(self_type&&) = default;

        /**
         * Mutable access operator for updating the logical value at index.
         *
         * @param i The index of the element to access.
         * @return Mutable proxy reference to the element at the specified index.
         */
        [[nodiscard]] SPARROW_API reference operator[](std::uint64_t i);

        /**
         * Constant access operator for getting element at index.
         *
         * @param i The index of the element to access.
         * @return Constant reference to the element at the specified index.
         */
        [[nodiscard]] SPARROW_API array_traits::const_reference operator[](std::uint64_t i) const;

        /**
         * Gets an iterator to the beginning of the array.
         *
         * @return Iterator to the beginning.
         */
        [[nodiscard]] SPARROW_API iterator begin();


        /**
         * Gets an iterator to the end of the array.
         *
         * @return Iterator to the end.
         */
        [[nodiscard]] SPARROW_API iterator end();

        /**
         * Gets a constant iterator to the beginning of the array.
         *
         * @return Constant iterator to the beginning.
         */
        [[nodiscard]] SPARROW_API const_iterator begin() const;

        /**
         * Gets a constant iterator to the end of the array.
         *
         * @return Constant iterator to the end.
         */
        [[nodiscard]] SPARROW_API const_iterator end() const;

        /**
         * Gets a constant iterator to the beginning of the array.
         *
         * @return Constant iterator to the beginning.
         */
        [[nodiscard]] SPARROW_API const_iterator cbegin() const;

        /**
         * Gets a constant iterator to the end of the array.
         *
         * @return Constant iterator to the end.
         */
        [[nodiscard]] SPARROW_API const_iterator cend() const;

        /**
         * Gets a reverse iterator to the beginning of the reversed array.
         *
         * @return Reverse iterator to the beginning.
         */
        [[nodiscard]] SPARROW_API reverse_iterator rbegin();

        /**
         * Gets a reverse iterator to the end of the reversed array.
         *
         * @return Reverse iterator to the end.
         */
        [[nodiscard]] SPARROW_API reverse_iterator rend();

        /**
         * Gets a constant reverse iterator to the beginning of reversed the array.
         *
         * @return Constant reverse iterator to the beginning.
         */
        [[nodiscard]] SPARROW_API const_reverse_iterator rbegin() const;

        /**
         * Gets a constant reverse iterator to the end of the reversed array.
         *
         * @return Constant reverse iterator to the end.
         */
        [[nodiscard]] SPARROW_API const_reverse_iterator rend() const;

        /**
         * Gets a constant reverse iterator to the beginning of reversed the array.
         *
         * @return Constant reverse iterator to the beginning.
         */
        [[nodiscard]] SPARROW_API const_reverse_iterator crbegin() const;

        /**
         * Gets a constant reverse iterator to the end of the reversed array.
         *
         * @return Constant reverse iterator to the end.
         */
        [[nodiscard]] SPARROW_API const_reverse_iterator crend() const;

        /**
         * Gets a mutable reference to the first element.
         *
         * @return Mutable proxy reference to the first element.
         */
        [[nodiscard]] SPARROW_API reference front();

        /**
         * Gets a constant reference to the first element.
         *
         * @return Constant reference to the first element.
         */
        [[nodiscard]] SPARROW_API array_traits::const_reference front() const;

        /**
         * Gets a mutable reference to the last element.
         *
         * @return Mutable proxy reference to the last element.
         */
        [[nodiscard]] SPARROW_API reference back();

        /**
         * Gets a reference to the last element.
         *
         * @return Constant reference to the last element.
         */
        [[nodiscard]] SPARROW_API array_traits::const_reference back() const;

        /**
         * Inserts a single logical value before pos.
         *
         * Complexity: $O(R_t)$ where $R_t$ is the number of encoded runs at or after pos.
         */
        SPARROW_API iterator insert(const_iterator pos, const value_type& value);

        /**
         * Inserts count copies of value before pos.
         *
         * This is implemented as a single structural mutation on the encoded runs, rather than
         * repeating single-element inserts.
         *
         * Complexity: $O(R_t)$ where $R_t$ is the number of encoded runs at or after pos.
         */
        SPARROW_API iterator insert(const_iterator pos, const value_type& value, size_type count);

        template <std::input_iterator InputIt>
            requires std::constructible_from<value_type, typename std::iterator_traits<InputIt>::value_type>
        iterator insert(const_iterator pos, InputIt first, InputIt last)
        {
            const size_type index = static_cast<size_type>(std::distance(cbegin(), pos));
            size_type inserted_count = 0;
            while (first != last)
            {
                value_type current_value(*first);
                ++first;
                insert_logical_value(index + inserted_count, current_value, first == last);
                ++inserted_count;
            }
            return sparrow::next(begin(), static_cast<std::ptrdiff_t>(index));
        }

        template <std::ranges::input_range R>
            requires std::constructible_from<value_type, std::ranges::range_value_t<R>>
        iterator insert(const_iterator pos, R&& range)
        {
            return insert(
                pos,
                std::ranges::begin(std::forward<R>(range)),
                std::ranges::end(std::forward<R>(range))
            );
        }

        /**
         * Erases the logical value at pos.
         *
         * Complexity: $O(R_t)$ where $R_t$ is the number of encoded runs touched by, or after, pos.
         */
        SPARROW_API iterator erase(const_iterator pos);

        /**
         * Erases the logical range [first, last).
         *
         * This is implemented as a single structural mutation on the encoded runs, rather than
         * repeating single-element erases.
         *
         * Complexity: $O(R_t)$ where $R_t$ is the number of encoded runs touched by, or after, the erased
         * range.
         */
        SPARROW_API iterator erase(const_iterator first, const_iterator last);

        SPARROW_API void push_back(const value_type& value);

        SPARROW_API void pop_back();

        SPARROW_API void resize(size_type new_length, const value_type& value);

        SPARROW_API void clear();

        /**
         * Checks if the array is empty.
         *
         * @return true if the array is empty, false otherwise.
         */
        [[nodiscard]] SPARROW_API bool empty() const;

        /**
         * Gets the number of elements in the array.
         *
         * @return The number of elements.
         */
        [[nodiscard]] SPARROW_API size_type size() const;

        /**
         * Gets the name of the array.
         *
         * @return Optional name of the array.
         */
        [[nodiscard]] std::optional<std::string_view> name() const;

        /**
         * Gets the metadata of the array.
         *
         * @return Optional metadata of the array.
         */
        [[nodiscard]] std::optional<key_value_view> metadata() const;

    private:

        /**
         * Creates an arrow proxy from run-ends and values children arrays.
         *
         * @tparam METADATA_RANGE The metadata container type.
         * @param acc_lengths The array containing the accumulated lengths (run-ends)
         * @param encoded_values The array containing the values
         * @param name Optional name for the array.
         * @param metadata Optional metadata for the array.
         * @return An arrow proxy containing the run-end encoded array data.
         */
        template <input_metadata_container METADATA_RANGE = std::vector<metadata_pair>>
        [[nodiscard]] static auto create_proxy(
            array&& acc_lengths,
            array&& encoded_values,
            std::optional<std::string_view> name = std::nullopt,
            std::optional<METADATA_RANGE> metadata = std::nullopt
        ) -> arrow_proxy;

        using acc_length_ptr_variant_type = std::variant<const std::int16_t*, const std::int32_t*, const std::int64_t*>;

        /**
         * Extracts the logical length and null count of the run-end encoded array
         * from its children.
         *
         * @param acc_lenghts_arr The child array containing the run-ends.
         * @param encoded_values_arr The child array containing the values.
         * @return a pair containing the length and the null count of the array.
         */
        [[nodiscard]] SPARROW_API static std::pair<std::int64_t, std::int64_t>
        extract_length_and_null_count(const array& acc_lengths_arr, const array& encoded_values_arr);

        /**
         * Returns a pointer to the data buffer containing the run-ends.
         *
         * @param ar The child array containing the rnu-ends.
         * @return A pointer to the data buffer containing the run-ends.
         */
        [[nodiscard]] SPARROW_API static acc_length_ptr_variant_type get_acc_lengths_ptr(const array& ar);

        /**
         * Gets the run-end value at the given index as an unsigned 64-bits integer.
         *
         * @param run_index the index in the run-end array.
         * @return run-end value as an unsigned 64-bits integer.
         */
        [[nodiscard]] SPARROW_API std::uint64_t get_acc_length(std::uint64_t run_index) const;

        /**
         * Gets a reference to the internal arrow proxy.
         *
         * @return Reference to the arrow proxy.
         */
        [[nodiscard]] SPARROW_API arrow_proxy& get_arrow_proxy();

        /**
         * Gets a constant reference to the internal arrow proxy.
         *
         * @return Constant reference to the arrow proxy.
         */
        [[nodiscard]] SPARROW_API const arrow_proxy& get_arrow_proxy() const;

        [[nodiscard]] SPARROW_API size_type find_run_index(std::uint64_t logical_index) const;

        [[nodiscard]] SPARROW_API std::uint64_t run_start(size_type run_index) const;

        [[nodiscard]] SPARROW_API std::uint64_t run_end(size_type run_index) const;

        [[nodiscard]] SPARROW_API const_reference encoded_value(size_type run_index) const;

        [[nodiscard]] SPARROW_API bool
        encoded_values_equal(size_type lhs_run_index, size_type rhs_run_index) const;

        [[nodiscard]] SPARROW_API bool encoded_value_equals(size_type run_index, const value_type& value) const;

        SPARROW_API void insert_encoded_value(size_type run_index, const value_type& value);

        SPARROW_API void erase_encoded_values(size_type run_index, size_type count);

        SPARROW_API void rebind_children_from_proxy();

        SPARROW_API void insert_acc_length(size_type pos, std::uint64_t value);

        SPARROW_API void erase_acc_lengths(size_type pos, size_type count);

        SPARROW_API void set_acc_length(size_type index, std::uint64_t value);

        SPARROW_API void shift_acc_lengths(size_type start_index, std::int64_t delta);

        SPARROW_API void merge_adjacent_runs(size_type left_run_index);

        SPARROW_API void refresh_and_merge_adjacent_runs(std::optional<size_type> merge_candidate);

        SPARROW_API void
        insert_logical_values(size_type index, const value_type& value, size_type count, bool refresh_state = true);

        SPARROW_API void erase_logical_values(size_type index, size_type count, bool refresh_state = true);

        SPARROW_API void
        insert_logical_value(size_type index, const value_type& value, bool refresh_state = true);

        SPARROW_API void erase_logical_value(size_type index, bool refresh_state = true);

        SPARROW_API void replace_logical_value(size_type index, const value_type& value);

        SPARROW_API void refresh_cache();

        SPARROW_API void refresh_after_mutation();

        SPARROW_API void finalize_mutation(bool refresh_state);

        SPARROW_API void throw_if_sliced_for_mutation(const char* operation) const;

        [[nodiscard]] SPARROW_API static value_type materialize_value(const const_reference& value);

        /** The arrow proxy containing the array data and schema. */
        arrow_proxy m_proxy;
        /** The length of therun-ends child array **/
        std::uint64_t m_encoded_length;

        /** The child array containing the run ends **/
        array p_acc_lengths_array;
        /** The child array containing the values **/
        array p_encoded_values_array;
        /** A pointer to the run-end child data buffer **/
        acc_length_ptr_variant_type m_acc_lengths;

        // friend classes
        friend class run_encoded_array_iterator<false>;
        friend class run_encoded_array_iterator<true>;
        friend class run_end_encoded_reference;
        friend class detail::array_access;
    };

    SPARROW_API
    bool operator==(const run_end_encoded_array& lhs, const run_end_encoded_array& rhs);

    template <input_metadata_container METADATA_RANGE>
    auto run_end_encoded_array::create_proxy(
        array&& acc_lengths,
        array&& encoded_values,
        std::optional<std::string_view> name,
        std::optional<METADATA_RANGE> metadata
    ) -> arrow_proxy
    {
        const auto flags = detail::array_access::get_arrow_proxy(encoded_values).flags();
        auto [null_count, length] = extract_length_and_null_count(acc_lengths, encoded_values);

        auto [acc_length_array, acc_length_schema] = extract_arrow_structures(std::move(acc_lengths));
        auto [encoded_values_array, encoded_values_schema] = extract_arrow_structures(std::move(encoded_values));

        constexpr auto n_children = 2;
        ArrowSchema** child_schemas = new ArrowSchema*[n_children];
        ArrowArray** child_arrays = new ArrowArray*[n_children];

        child_schemas[0] = new ArrowSchema(acc_length_schema);
        child_schemas[1] = new ArrowSchema(encoded_values_schema);

        child_arrays[0] = new ArrowArray(acc_length_array);
        child_arrays[1] = new ArrowArray(encoded_values_array);

        const repeat_view<bool> children_ownserhip{true, n_children};

        ArrowSchema schema = make_arrow_schema(
            std::string("+r"),
            std::move(name),      // name
            std::move(metadata),  // metadata
            flags,                // flags,
            child_schemas,        // children
            children_ownserhip,   // children ownership
            nullptr,              // dictionary
            true                  // dictionary ownership
        );

        std::vector<buffer<std::uint8_t>> arr_buffs = {};

        ArrowArray arr = make_arrow_array(
            static_cast<std::int64_t>(length),  // length
            static_cast<int64_t>(null_count),
            0,  // offset
            std::move(arr_buffs),
            child_arrays,        // children
            children_ownserhip,  // children ownership
            nullptr,             // dictionary
            true                 // dictionary ownership
        );

        return arrow_proxy{std::move(arr), std::move(schema)};
    }

}  // namespace sparrow


#if defined(__cpp_lib_format)

template <>
struct std::formatter<sparrow::run_end_encoded_array>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();  // Simple implementation
    }

    SPARROW_API auto format(const sparrow::run_end_encoded_array& ar, std::format_context& ctx) const
        -> decltype(ctx.out());
};

namespace sparrow
{
    SPARROW_API std::ostream& operator<<(std::ostream& os, const run_end_encoded_array& value);
}

#endif
