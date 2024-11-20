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

#include "sparrow/c_interface.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/layout/array_access.hpp"
#include "sparrow/layout/array_wrapper.hpp"
#include "sparrow/layout/nested_value_types.hpp"
#include "sparrow/layout/null_array.hpp"
#include "sparrow/types/data_traits.hpp"

namespace sparrow
{
    /**
     * Dynamically typed array encapsulating an Arrow layout.
     *
     * The \c array class is a dynamically typed container that encapsulates
     * a typed Arrow layout. It provides accessors returning a variant of the
     * suported data types, and supports the visitor pattern.
     *
     * This class is designed to easily manipulate data from Arrow C structures
     * and to easily extract Arrow C structures from layouts allocated with sparrow.
     * It supports different models of ownership.
     */
    class array
    {
    public:

        using size_type = std::size_t;
        using value_type = array_traits::value_type;
        using const_reference = array_traits::const_reference;
 
        /**
         * Constructs an empty array.
         */
        SPARROW_API array() = default;

        /**
         * Constructs an array from the given typed layout. The ownership
         * of the layout is transferred to the array.
         *
         * @param a An rvalue reference to the typed layout.
         */
        template <layout A>
        requires (not std::is_lvalue_reference_v<A>)
        explicit array(A&& a);

        /**
         * Constructs an array from the given typed layout. The ownership of
         * the layout is not transferred to the array and the layout's lifetime
         * must be longer than that of the array.
         *
         * @param a A pointer to the typed layout.
         */
        template <layout A>
        explicit array(A* a);

        /**
         * Constructs an array from the given typed layout. The ownership of
         * the layout is shared by this array and any other shared pointer
         * referencing it.
         *
         * @param a A shared pointer holding the layout.
         */
        template <layout A>
        explicit array(std::shared_ptr<A> a);

        /**
         * Constructs an array from the given Arrow C structures, whose ownerhips
         * is transferred to the array. The user should not use \c array nor \c schema
         * after calling this constructor.
         *
         * @param array The ArrowArray structure to transfer into the array.
         * @param schema The ArowSchema structure to transfer into the array.
         */
        SPARROW_API array(ArrowArray&& array, ArrowSchema&& schema);

        /**
         * Constructs an array from the given Arrow C structures. The array takes
         * the ownerhship of the ArrowArray only. The used should not use \c array
         * after calling this constructor. \c shcema can still be used normally.
         *
         * @param array The ArrowArray structure to transfer into the array.
         * @param schema The ArrowSchema to reference in the array.
         */
        SPARROW_API array(ArrowArray&& array, ArrowSchema* schema);

        /**
         * Constructs an array from the given Arrow C structures. Both structures
         * are referenced from the array and can still be used normally after calling
         * this constructor.
         *
         * @param array The arrowArray structure to reference in the array.
         * @param schema The ArrowSchema to reference in the array.
         */
        SPARROW_API array(ArrowArray* array, ArrowSchema* schema);
        
        /**
         * @returns the data type of the array.
         */
        SPARROW_API enum data_type data_type() const;

        /**
         * Checks if the array has no element, i.e. whether size() == 0.
         */
        SPARROW_API bool empty() const;

        /**
         * @returns the number of elements in the array.
         */
        SPARROW_API size_type size() const;

        /**
         * @returns a constant reference to the element at specified \c index,
         * with bounds checking.
         *
         * @param index The position of the element in the array.
         * @throw std::out_of_range if \p index is not within the range of the container.
         */
        SPARROW_API const_reference at(size_type index) const;

        /**
         * @returns a constant reference to the element at specified \c index.
         *
         * @param index The position of the element in the array.
         */
        SPARROW_API const_reference operator[](size_type index) const;

        /**
         * Returns a constant reference to the first element in the container.
         * Calling `front` on an empty array causes undefined behavior.
         */
        SPARROW_API const_reference front() const;

        /**
         * Returns a constant reference to the last element in the container.
         * Calling `back` on an empty array causes undefined behavior.
         */
        SPARROW_API const_reference back() const;

        template <class F>
        using visit_result_t = std::invoke_result_t<F, null_array>;
 
        /**
         * Returns the result of calling the given functor \c func on the
         * layout internally hold by the array. The actual type of the
         * layout is retrieved via a visitor dispatch. \c func must
         * accept any kind of layout.
         *
         * @param func The functor to apply.
         * @return The result of calling the functor on the internal layout.
         */       
        template <class F>
        visit_result_t<F> visit(F&& func) const;

    private:

        SPARROW_API arrow_proxy& get_arrow_proxy();
        SPARROW_API const arrow_proxy& get_arrow_proxy() const;

        cloning_ptr<array_wrapper> p_array = nullptr;

        friend class detail::array_access;
    };

    /**
     * Compares the content of two arrays.
     *
     * @param lhs the first array to compare
     * @param rhs the second array to compare
     * @return \c true if the contents of both arrays
     * are equal, \c false otherwise.
     */
    SPARROW_API
    bool operator==(const array& lhs, const array& rhs);

    template <class A>
    concept layout_or_array = layout<A> or std::same_as<A, array>;

    /**
     * Returns true if the given layout or array has ownership
     * of its internal ArrowArray.
     *
     * @param a An array or a typed layout object.
     * @return \c true if \c a owns its internal ArrowArray, false
     * otherwise.
     */
    template <layout_or_array A>
    bool owns_arrow_array(const A& a);

    /**
     * Returns true if the given layout or array has ownership
     * of its internal ArrowSchema.
     *
     * @param a An array or a typed layout object.
     * @returns \c true if \c a owns its internal ArrowSchema, false
     * otherwise.
     */
    template <layout_or_array A>
    bool owns_arrow_schema(const A& a);

    /**
     * Returns a pointer to the internal ArrowArray of the given
     * array or layout.
     *
     * @param a An array or a typed layout.
     * @returns a pointer to the internal ArrowArray.
     */
    template <layout_or_array A>
    ArrowArray* get_arrow_array(A& a);

    /**
     * Returns a pointer to the internal ArrowSchema of the given
     * array or layout.
     *
     * @param a An array or a typed layout.
     * @returns a pointer to the internal ArrowSchema.
     */
    template <layout_or_array A>
    ArrowSchema* get_arrow_schema(A& a);

    /**
     * Returns pointers to the internal ArrowArray and ArowSchema of
     * the given array or layout.
     *
     * @param a An array or a typed layout.
     * @returns pointers to the internal ArrowArray and ArrowSchema.
     */
    template <layout_or_array A>
    std::pair<ArrowArray*, ArrowSchema*> get_arrow_structures(A& a);

    /**
     * Extracts the internal ArrowArray structure from the given array
     * or typed layout. After this call, the user is responsible for
     * the management of the returned ArrowArray.
     *
     * Throws a \c std::runtime_error if \c a does not own its internal
     * ArrowArray before this call.
     *
     * @param a An array or a typed layout.
     * @returns The internal ArrowArray.
     */
    template <layout_or_array A>
    ArrowArray extract_arrow_array(A&& a);

    /**
     * Extracts the internal ArrowSchema structure from the given array
     * or typed layout. After this call, the user is responsible for
     * the management of the returned ArrowSchema.
     *
     * Throws a \c std::runtime_error if \c a does not own its internal
     * ArrowSchema before this call.
     *
     * @param a An array or a typed layout.
     * @returns The internal ArrowSchema.
     */
    template <layout_or_array A>
    ArrowSchema extract_arrow_schema(A&& a);

    /**
     * Extracts the internal ArrowArrays and ArrowSchema structures from
     * the given array or typed layout. After this call, the user is
     * responsible for the management of the returned ArrowArray and
     * ArrowSchema.
     *
     * Throws a \c std::runtime_error if \c a does not own its internal
     * ArrowArray and ArrowSchema before this call.
     *
     * @param a An array or a typed layout.
     * @returns The internal ArrowArray and ArrowSchema.
     */
    template <layout_or_array A>
    std::pair<ArrowArray, ArrowSchema> extract_arrow_structures(A&& a);
}

