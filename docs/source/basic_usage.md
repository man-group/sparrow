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
#include "sparrow/sparrow.hpp"
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
#include "sparrow/sparrow.hpp"
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
#include "sparrow/sparrow.hpp"
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
#include "sparrow/sparrow.hpp"
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
#include "sparrow/sparrow.hpp"
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

For performance-critical applications, Sparrow provides zero copy constructors that avoid unnecessary data copying. When constructing dictionary encoded arrays and other specialized array types, you can achieve zero copy operations by using constructors that accept `sparrow::u8_buffer` as input parameters. The `u8_buffer` type is specifically designed to wrap existing memory buffers without performing any data copying, allowing you to directly use pre-allocated memory regions.

When using constructors that take ranges or standard containers as input, Sparrow typically needs to copy the data into its internal buffer structures. However, constructors accepting `sparrow::u8_buffer` parameters can take ownership of existing memory buffers or create views over them, eliminating the need for data copying. This is particularly beneficial when working with large datasets or when integrating with external libraries that already have data in the appropriate memory layout.

To utilize zero copy operations, construct your arrays using the overloads that accept `sparrow::u8_buffer` objects for buffer parameters such as keys, values, or validity bitmaps. This approach provides optimal performance by reusing existing memory allocations while maintaining Sparrow's type safety and API consistency.

