Basic usage               {#basic_usage}
===========

Initialize data with sparrow ...
--------------------------------

The two following examples initialize a sparrow array and then get the
internal Arrow C structures. The only difference is the ownership of
these structures.

### ... and extract C data structures

In this example, we take ownership of the Arrow structures allocated
by the sparrow array. This latter must not be used after, and it is our
responsibility to release the C structures when we don't need them anymore.

```cpp
#include "sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> ar = { 1, 3, 5, 7, 9 };
auto [arrow_array, arrow_schema] = sp::extract_arrow_structures(std::move(ar));
// Use arrow_array and arrow_schema as you need (serialization, passing them to
// a third party library)
// ...
// You are responsible for releasing the structures at the end
arrow_array.release(&arrow_array);
arrow_schema.release(&arrow_schema);
```

### ... and use C data structures

In this example, the sparrow array keeps the ownership of its internal Arrow
structures, making it possible to continue using it independently. We must
NOT release the Arrow structures after we don't need them; the sparrow array
will do it.

```cpp
#include "sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> ar = { 1, 3, 5, 7, 9 };
// Caution: get_arrow_structures returns pointers, not values
auto [arrow_array, arrow_schema] = sp::get_arrow_structures(ar);
// Use arrow_array and arrow_schema as you need (serialization, passing them to
// a third party library)
// ...
// Do NOT release the C structures at the end; the "ar" variable will do it for you
```

Read data from somewhere ...
----------------------------

In the following examples, we read Arrow data via a third party library and we get
Arrow C structures. We use them to initialize a sparrow (untyped) array. Like the
previous examples, the only difference is how we handle the ownership of the C
structures.

### ... and pass it to sparrow

In this example, we keep the ownership of the Arrow structures. The sparrow array
will simply reference them internally, but it will not release them when it goes out
of scope. We are responsible for doing it.

```cpp
#include "sparrow.hpp"
#include "third-party-lib.hpp"
namespace sp = sparrow;
namespace tpl = third_party_library;

ArrowArray array;
ArrowSchema schema;
tpl::read_arrow_structures(&array, &schema);

sp::array ar(&array, &schema);
// Use ar as you need
// ...
// You are responsible for releasing the structures at the end
array.release(&array);
schema.release(&schema);
```

### ... and move it into sparrow

In this example, we transfer the ownership of the Arrow C structures
to the sparrow array. It will release them when it is destroyed.

```cpp
#include "sparrow.hpp"
#include "third-party-lib.hpp"
namespace sp = sparrow;
namespace tpl = third_party_library;

ArrowArray array;
ArrowSchema schema;
tpl::read_arrow_structures(&array, &schema);

sp::array ar(std::move(array), std::move(schema));
// Use ar as you need
// ...
// Do NOT release the C structures at the end; the "ar" variable will do it for you
```

Apply an algorithm to an untyped array
--------------------------------------

This example demonstrates how to apply an algorithm to an untyped array using the
\ref sparrow::array::visit method. This method determines the type of the internal array that
stores the data and passes it to the user-defined functor. As a result, the functor must
be designed to handle any typed array provided by Sparrow.

```cpp
#include "sparrow.hpp"
#include "third-party-lib.hpp"
namespace sp = sparrow;
namespace tpl = third_party_library;

ArrowArray array;
ArrowSchema schema;
tpl::read_arrow_structures(&array, &schema);

sp::array ar(std::move(array), std::move(schema));
ar.visit([]<class T>(const T& typed_ar)
{
    if constexpr (sp::is_primitive_array_v<T>)
    {
        std::for_each(typed_ar.cbegin(), typed_ar.cend(), [](const auto& val)
        {
            if (val.has_value())
            {
                std::cout << val.value();
            }
            else
            {
                std::cout << "null";
            }
            std::cout << ", ";
        });
    }
    // else if constexpr ...
});
```

Zero Copy Operations
--------------------

For performance-critical applications, Sparrow provides zero copy constructors that avoid unnecessary data copying. When constructing arrays, you can achieve zero copy operations by using constructors that accept `sparrow::u8_buffer` as input parameters. The `u8_buffer` type is specifically designed to wrap existing memory buffers without performing any data copying, allowing you to directly use pre-allocated memory regions.

When using constructors that take ranges or standard containers as input, Sparrow typically needs to copy the data into its internal buffer structures. However, constructors accepting `sparrow::u8_buffer` parameters can take ownership of existing memory buffers or create views over them, eliminating the need for data copying. This is particularly beneficial when working with large datasets or when integrating with external libraries that already have data in the appropriate memory layout.

### Transferring Ownership of Existing Memory

Sparrow uses `xsimd::aligned_allocator` by default for optimal performance with SIMD operations. When transferring ownership of an existing memory buffer to Sparrow, you must provide an allocator that matches how the memory was originally allocated. This ensures proper deallocation when the buffer is destroyed.

```cpp
#include "sparrow.hpp"
#include "sparrow/details/3rdparty/xsimd_aligned_allocator.hpp"
namespace sp = sparrow;

// Allocate memory using aligned allocator
using allocator_type = xsimd::aligned_allocator<int>;
allocator_type alloc;
int* ptr = alloc.allocate(100);

// Initialize the data
for (size_t i = 0; i < 100; ++i)
{
    ptr[i] = static_cast<int>(i);
}

// Transfer ownership to sparrow buffer with the matching allocator
// The buffer will deallocate the memory using the same allocator
sp::buffer<int> buf(ptr, 100, alloc);

// Use the buffer as needed
// ...
// Memory is automatically released when buf goes out of scope
```

If you have memory allocated with the `new` operator or other custom allocation methods, you must provide a compatible custom allocator that will properly deallocate the memory. Here's an example of a custom allocator for `new[]` allocated memory:

```cpp
#include "sparrow.hpp"
namespace sp = sparrow;

// Custom allocator for memory allocated with new[]
template <typename T>
struct new_delete_allocator
{
    using value_type = T;
    
    T* allocate(std::size_t n)
    {
        return new T[n];
    }
    
    void deallocate(T* p, std::size_t)
    {
        delete[] p;
    }
};

// Allocate memory using new operator
int* ptr = new int[100];

// Initialize the data
for (size_t i = 0; i < 100; ++i)
{
    ptr[i] = static_cast<int>(i);
}

// Transfer ownership to sparrow buffer with custom allocator
new_delete_allocator<int> alloc;
sp::buffer<int> buf(ptr, 100, alloc);

// Use the buffer as needed
// ...
// Memory is automatically released with delete[] when buf goes out of scope
```

**Important:** The allocator passed to the buffer constructor must be compatible with the allocation method used for the pointer. The allocator's `deallocate()` method will be called to free the memory, so you must ensure it matches how the memory was allocated. Using an incompatible allocator will result in undefined behavior during deallocation.

Floating-Point Type Traits
--------------------------

When writing generic code that operates on Sparrow's floating-point types, you must use Sparrow's own type traits rather than the standard library ones. This section explains why, and how to use them correctly.

### Background

Sparrow supports three fixed-width floating-point types:

| Sparrow alias        | C++23 standard type | Fallback (pre-C++23) |
|----------------------|---------------------|----------------------|
| `sparrow::float16_t` | `std::float16_t`    | `half_float::half`   |
| `sparrow::float32_t` | `std::float32_t`    | `float`              |
| `sparrow::float64_t` | `std::float64_t`    | `double`             |

When the compiler and standard library support C++23 fixed-width floating-point types (detected via `__STDCPP_FLOAT16_T__`, `__STDCPP_FLOAT32_T__`, and `__STDCPP_FLOAT64_T__`), Sparrow defines the macro `SPARROW_STD_FIXED_FLOAT_SUPPORT` and maps these aliases directly to the standard types. In that case, the standard type traits work correctly for all three types.

When `SPARROW_STD_FIXED_FLOAT_SUPPORT` is **not** defined, `sparrow::float16_t` maps to `half_float::half` — a third-party 16-bit floating-point type that the standard library does not recognise. As a result, `std::is_floating_point<half_float::half>::value` returns `false`, `std::is_scalar<half_float::half>::value` returns `false`, and `std::is_signed<half_float::half>::value` also returns `false`.

### Sparrow Trait Replacements

To handle this portably, Sparrow defines its own traits (only when `SPARROW_STD_FIXED_FLOAT_SUPPORT` is **not** defined):

```cpp
// In namespace sparrow:
template <class T> struct is_floating_point;  // forwards to std::is_floating_point<T>,
                                               // specialised to true for half_float::half
template <class T> struct is_scalar;           // forwards to std::is_scalar<T>,
                                               // specialised to true for half_float::half
template <class T> struct is_signed;           // forwards to std::is_signed<T>,
                                               // specialised to true for half_float::half

template <class T> inline constexpr bool is_floating_point_v = is_floating_point<T>::value;
template <class T> inline constexpr bool is_scalar_v         = is_scalar<T>::value;
template <class T> inline constexpr bool is_signed_v         = is_signed<T>::value;
```

For all types other than `half_float::half`, these traits delegate directly to the corresponding `std::` traits, so using them incurs no behavioural difference for standard types.

### Usage

Always prefer `sparrow::is_floating_point_v` (and the other `sparrow::` traits) over their `std::` counterparts in any template code that may be instantiated with a Sparrow floating-point type:

```cpp
#include "sparrow.hpp"
namespace sp = sparrow;

template <typename T>
void process(T value)
{
    // Correct: works for float, double, and sparrow::float16_t on all compilers
    if constexpr (sp::is_floating_point_v<T>)
    {
        // floating-point specific logic
    }

    // Wrong: may silently fail for sparrow::float16_t when SPARROW_STD_FIXED_FLOAT_SUPPORT
    // is not defined (half_float::half is not recognised by std::is_floating_point)
    // if constexpr (std::is_floating_point_v<T>) { ... }
}
```

The check needed to guard code that uses these traits in a conditional compilation context:

```cpp
#include "sparrow/types/data_type.hpp"

template <typename T>
void example(T value)
{
#if defined(SPARROW_STD_FIXED_FLOAT_SUPPORT)
    // std traits work correctly; use either std:: or sp:: variants
    static_assert(std::floating_point<sp::float16_t>);
#else
    // Must use sparrow traits so that half_float::half is handled correctly
    static_assert(sp::is_floating_point_v<sp::float16_t>);
#endif
}
```

### Rationale

These Sparrow traits are intentionally defined **only** when `SPARROW_STD_FIXED_FLOAT_SUPPORT` is absent — that is, only when the compiler does not yet provide C++23 fixed-width floating-point support. Once a codebase moves to C++23, `sparrow::float16_t` becomes `std::float16_t`, and the standard traits work correctly for it, so the Sparrow overrides are no longer needed.

Making the Sparrow traits available unconditionally might seem convenient (it would mean client code never has to think about the macro), but it would also mask the eventual migration path: if the Sparrow traits were always present, there would be no compile-time signal encouraging users to switch back to the standard `std::` traits once C++23 support is available. The current design therefore keeps the Sparrow traits scoped to exactly those platforms where they are necessary, acting as a temporary shim rather than a permanent parallel API.

**Summary of rules for client code:**

- When writing template code involving `sparrow::float16_t`, `sparrow::float32_t`, or `sparrow::float64_t`, use `sparrow::is_floating_point_v`, `sparrow::is_scalar_v`, and `sparrow::is_signed_v` instead of their `std::` counterparts.
- Sparrow cannot enforce that you use these traits at compile time, so discipline on the client side is required.
- Once your toolchain fully supports C++23 fixed-width floating-point types (i.e. `SPARROW_STD_FIXED_FLOAT_SUPPORT` is defined), you can migrate back to the standard `std::` traits.

