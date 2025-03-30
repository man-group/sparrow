Typed arrays      {#typed_arrays}
============

`sparrow` offers an array class for each Arrow layout. These arrays are commonly referred to as
typed arrays because the exact data type is known. Despite differences in memory layout, all these
arrays share a consistent API for reading data. This API is designed to resemble that of the standard
container [std::vector](https://en.cppreference.com/w/cpp/container/vector).

Common API    {#common_apis}
----------

### Support for null values

Typed arrays support missing values, or "nulls": any value in an array may be semantically null,
regardless of its type.Therefore, when reading data from an array, one does not get scalar values,
but a value of a special type that can handle missing values: \ref nullable.

\ref nullable has an API similar to [std::optional](https://en.cppreference.com/w/cpp/utility/optional):

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::nullable<int> n = 2;
std::cout << n.has_value() << std::endl; // Prints true
std::cout << n.value() << std::endl;     // Prints 2

sp::nullable<double> nd = sp::nullval;
std::cout << nd.has_value() << std::endl; // Prints false
```

Contrary to [std::optional](https://en.cppreference.com/w/cpp/utility/optional), \ref nullable
can hold a reference and act as a reference proxy. Assigning to a nullable proxy will assign to
referenced value, it won't rebind the internal reference:

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

int i = 5;
sp::nullable<int&> n = i;
n = 7;
std::cout << i << std::endl; // Prints 7
```

However, assigning null to a nullable proxy does not change the underlying value, it just reset the internal
flags:

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

int i = 5;
sp::nullable<int&> n = i;
n = sp::nullval;
std::cout << i << std::endl; // Prints 5
```

### Capacity

Typed arrays provide the following methods regarding their capacity:

| Method | Description                           |
| ------ | ------------------------------------- |
| empty  | Checks whether the container is empty |
| size   | Returns the number of elements        |

Example:

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = { 1, 2, 3, 4};
std::cout << pa.size() << std::endl;  // Prints 4
std::cout << pa.empty() << std::endl; // Prints false
```

### Element access

Typed arrays provide the following const methods to read elements:

| Method     | Description                                   |
| ---------- | --------------------------------------------- |
| at         | Access specified element with bounds checking |
| operator[] | Access specified element                      |
| front      | Access the first element                      |
| back       | Access the last element                       |

For an array holding data of type `T`, these methods return values
of type `nullable<const T&>`.

Example:

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = { 1, 2, 3, 4};
std::cout << pa.front().value() << std::endl; // Prints 1
std::cout << pa.back().value() << std::endl;  // Prints 4
std::cout << pa[2].value() << std::endl;      // Prints 3
std::cout << pa.at(5).value() << std::endl;   // Throws
```

### Iterators

Typed arrays also provide traditional iterating methods:

| Method  | Description                                 |
| ------- | ------------------------------------------- |
| begin   | Returns an iterator to the beginning        |
| cbegin  | Returns an iterator to the beginning        |
| end     | Returns an iterator to the end              |
| cend    | Returns an iterator to the end              |
| rbegin  | Returns a reverse iterator to the beginning |
| crbegin | Returns a reverse iterator to the beginning |
| rend    | Returns a reverse iterator to the end       |
| crend   | Returns a reverse iterator to the end       |

Example:

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = { 1, 2, 3, 4};
std::for_each(pa.begin(), pa.end(), [](auto n) { std::cout << n.value() << ' '; });
std::cout << '\n';
// Prints 1 2 3 4
```

Layout types  {#layout_types}
------------

In addition to the common API described above, typed arrays offer convenient
constructors tailored to their specific types. They also implement full value-semantics
and can therefore be copied and moved:

```cpp
#include "sparrow/sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa0 = { 1, 2, 3, 4};
sp::primitive_array<int> pa1 = { 2, 3, 4, 5};

sp::primitive_array<int> pa2(pa0);
sp::primitive_array<int> pa3(std::move(pa1));
pa2 = pa3;
pa3 = std::move(pa0);
```

### primitive_array

This array implements the [Fixed-Size Primitive Layout](https://arrow.apache.org/docs/format/Columnar.html#fixed-size-primitive-layout).

TODO: add constructors.

### variable_size_binary_array

This array implements the [Variable-Size Binary Layout](https://arrow.apache.org/docs/format/Columnar.html#variable-size-binary-layout).

TODO: add constructors.

### list_array

This array implements the [Variable-Size List Layout](https://arrow.apache.org/docs/format/Columnar.html#list-layout).

TODO: add constructors.

### list_view_array

This array implements the [Variable-size ListView Layout](https://arrow.apache.org/docs/format/Columnar.html#listview-layout).

TODO: add constructors.

### fixed_size_list_array

This array implements the [Fixed-size List Layout](https://arrow.apache.org/docs/format/Columnar.html#fixed-size-list-layout).

TODO: add constructors.

### struct_array

This array implements the [Struct Layout](https://arrow.apache.org/docs/format/Columnar.html#struct-layout).

TODO: add constructors.

### union_array

This array implements the [Union Layout](https://arrow.apache.org/docs/format/Columnar.html#union-layout).

TODO: add constructors.

### null_array

This array implements the [Union Layout](https://arrow.apache.org/docs/format/Columnar.html#union-layout).

TODO: add constructors.

### dictionary_encoded_array

This array implements the [Dictionary-encoded Layout](https://arrow.apache.org/docs/format/Columnar.html#dictionary-encoded-layout).

TODO: add constructors.
