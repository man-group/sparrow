Array      {#array}
=====

The `array` class is a dynamically typed array that can be built in many ways:
- either from an existing [typed array](#typed_arrays), that it will type-erased
- or from [Arrow C data interface](https://arrow.apache.org/docs/format/CDataInterface.html#structure-definitions).
The array can simply have references to the structures, or take their ownership.

Example:
```cpp
#include "sparrow/sparrow.hpp"
#include "thrid-party-lib.hpp"
namespace sp = sparrow;
namespace tpl = third_party_library;

ArrowArray arr, arr2;
ArrowSchema schema, schema2;
tpl::read_arrow_structures(&arr, &schema);
tpl::read_arrow_structures(&arr2, &schema2);

sp::array ar(&arr, &schema);
sp::array ar2(std::move(arr2), std::move(schema2));
// ...
arr.release(&arr);
schema.release(&schema);
// We don't release arr2 nor shcema2, ar2 will do it for us.
```

The array class provides a similar API to that of the [typed arrays](#common_apis), but with certain limitations:
iterators are not provided, for performance reasons. Instead, a method for visiting the array and apply
an algorithm to the undelying typed array is provided.

Array API
---------

### Capacity

Like typed arrays, array provides the following methods regarding its capacity:

| Method | Description                           |
| ------ | ------------------------------------- |
| empty  | Checks whether the container is empty |
| size   | Returns the number of elements        |

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = { 1, 2, 3, 4};
sp::array a(std::move(pa));
std::cout << a.size() << std::endl;  // Prints 4
std::cout << a.empty() << std::endl; // Prints false
```

### Element access

Accessing an element in an `array` yields a [std::variant](https://en.cppreference.com/w/cpp/utility/variant)
of \ref nullable objects. The variant can hold any data type used by the [typed array classes](#layout_types).
A method is provided so that the user can retrieve the dynamic data_type of the array.

| Method     | Description                                   |
| ---------- | --------------------------------------------- |
| data_type  | Returns the dynamic type of the data          |
| at         | Access specified element with bounds checking |
| operator[] | Access specified element                      |
| front      | Access the first element                      |
| back       | Access the last element                       |

Example:

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = { 1, 2, 3, 4};
sp::array ar(std::move(pa));
std::visit([](const auto& arg) { std::cout << arg << '\n'; }, ar.front());
std::visit([](const auto& arg) { std::cout << arg << '\n'; }, ar.back());
std::visit([](const auto& arg) { std::cout << arg << '\n'; }, ar[i]);
std:cout << sp::data_type_to_format(ar.data_type()) << std::endl;
```

### Visit

The visit function allows the user to apply a functor to each element of the array. The functor
must accept any kind of [typed array](#layout_types).

Example:

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = { 1, 2, 3, 4};
sp::array ar(std::move(pa));

std::visit([](const auto& arr)
{
    std::for_each(arr.begin(), ar.end(), [](const auto& val)
    {
        // Do whatever you need here
        // Keep in min val can be a primitive type,
        // a string, or even a nested array.
    });
}, ar);
```

Conversion to Arrow C data
--------------------------

sparrow provides free functions to either read data from sparrow arrays as
[Arrow C data](https://arrow.apache.org/docs/format/CDataInterface.html#structure-definitions),
or to extract them.

### Checking ownership

| Method                   | Description                                   |
| ------------------------ | --------------------------------------------- |
| owns_arrow_array         | Checks for internal Arrow Array ownership     |
| owns_arrow_schema        | Checks for internal Arrow Schema ownership    |

Example:
```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = { 1, 2, 3, 4};
sp::array ar(std::move(pa));
std::cout << owns_arrow_array(ar) << std::endl;
std::cout << owns_arrow_schema(ar) << std::endl;
```

### Reading

These methods return pointers to the internal Arrow structures. One must NOT release
the returned structures after using them.

| Method                   | Description                                                |
| ------------------------ | ---------------------------------------------------------- |
| get_arrow_array          | Returns a pointer to the internal Arrow Array              |
| get_arrow_schema         | Returns a pointer to the internal Arrow Schema             |
| get_arrow_structures     | Returns a pair of pointer to the internal Arrow structures |

Example:
```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = { 1, 2, 3, 4};
sp::array ar(std::move(pa));
ArrowArray* arr = get_arrow_array(ar);
ArrowSchema sch = get_arrow_schema(ar);
// OR
auto [arr, sch] = get_arrow_structures(ar);
```

### Extracting

These methods moves out of the array the internal Arrow structures. The user is responsible
for releasing them after use.

| Method                   | Description                            |
| ------------------------ | -------------------------------------- |
| extract_arrow_array      | Extracts the internal Arrow Array      |
| extract_arrow_schema     | Extracts the internal Arrow Schema     |
| extract_arrow_structures | Extracts the internal Arrow structures |

Example:
```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = { 1, 2, 3, 4};
sp::array ar(std::move(pa));
ArrowArray* arr = extract_arrow_array(std::move(ar));
ArrowSchema sch = extract_arrow_schema(std::move(ar));
// OR
auto [arr, sch] = extract_arrow_structures(std::move(ar));
// ...
arr.release(&arr);
sch.release(&sch);
```

