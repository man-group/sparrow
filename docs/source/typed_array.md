# Typed Arrays      {#typed_arrays}

`sparrow` offers an array class for each Arrow layout. These arrays are commonly referred to as
typed arrays because the exact data type is known at compile time. Despite differences in memory layout, all these
arrays share a consistent API for reading data. This API is designed to resemble that of the standard
container [std::vector](https://en.cppreference.com/w/cpp/container/vector).

## Common API    {#common_apis}

### Support for null values

Typed arrays support missing values, or "nulls": any value in an array may be semantically null,
regardless of its type. Therefore, when reading data from an array, one does not get scalar values,
but a value of a special type that can handle missing values: \ref sparrow::nullable.

\ref sparrow::nullable has an API similar to [std::optional](https://en.cppreference.com/w/cpp/utility/optional):

```cpp
#include "sparrow.hpp"
namespace sp = sparrow;

sp::nullable<int> n = 2;
std::cout << n.has_value() << std::endl; // Prints true
std::cout << n.value() << std::endl;     // Prints 2

sp::nullable<double> nd = sp::nullval;
std::cout << nd.has_value() << std::endl; // Prints false
```

Unlike [std::optional](https://en.cppreference.com/w/cpp/utility/optional), \ref sparrow::nullable
can hold a reference and act as a reference proxy. Assigning to a nullable proxy will assign to the
referenced value; it won't rebind the internal reference:

```cpp
#include "sparrow.hpp"
namespace sp = sparrow;

int i = 5;
sp::nullable<int&> n = i;
n = 7;
std::cout << i << std::endl; // Prints 7
```

However, assigning null to a nullable proxy does not change the underlying value; it just resets the internal
flag:

```cpp
#include "sparrow.hpp"
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
#include "sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = {1, 2, 3, 4};
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
#include "sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = {1, 2, 3, 4};
std::cout << pa.front().value() << std::endl; // Prints 1
std::cout << pa.back().value() << std::endl;  // Prints 4
std::cout << pa[2].value() << std::endl;      // Prints 3
try {
    std::cout << pa.at(5).value() << std::endl;   // Throws std::out_of_range
} catch (const std::out_of_range& e) {
    std::cout << "Index out of range" << std::endl;
}
```

### Iterators

Typed arrays also provide traditional iteration methods:

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
#include "sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa = {1, 2, 3, 4};
std::for_each(pa.begin(), pa.end(), [](auto n) { std::cout << n.value() << ' '; });
std::cout << '\n';
// Prints 1 2 3 4
```

## Layout Types  {#layout_types}

In addition to the common API described above, typed arrays offer convenient
constructors tailored to their specific types. They also implement full value semantics
and can therefore be copied and moved:

```cpp
#include "sparrow.hpp"
namespace sp = sparrow;

sp::primitive_array<int> pa0 = {1, 2, 3, 4};
sp::primitive_array<int> pa1 = {2, 3, 4, 5};

sp::primitive_array<int> pa2(pa0);
sp::primitive_array<int> pa3(std::move(pa1));
pa2 = pa3;
pa3 = std::move(pa0);
```

### Primitive Arrays

#### primitive_array<T>
**Header:** `sparrow/primitive_array.hpp`

**Description:** Stores scalar values of a specific type (integers, floats, booleans) in a contiguous memory layout.
The T type can be:
- `int8_t`, `int16_t`, `int32_t`, `int64_t`
- `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`
- `float16_t`, `float`, `double`
- `bool`, `std::byte`

**When to use:** For homogeneous collections of scalar values.

### Variable-Size Binary Arrays

#### string_array / big_string_array
**Header:** `sparrow/variable_size_binary_array.hpp`

**Description:** Stores variable-length strings or binary data. `big_string_array` uses 64-bit offsets for larger datasets.

**When to use:** For text data, variable-length strings, or binary blobs.

#### binary_array / big_binary_array
**Header:** `sparrow/variable_size_binary_array.hpp`

**Description:** Similar to string arrays but for binary data (sequences of bytes).

**When to use:** For variable-length binary data like images, serialized objects, or raw byte sequences.

### Fixed-Width Binary Arrays

#### fixed_width_binary_array
**Header:** `sparrow/fixed_width_binary_array.hpp`

**Description:** Stores fixed-size binary data where all elements have the same byte length.

**When to use:** For fixed-size binary data like UUIDs, hashes, or fixed-size encoded data.

### List Arrays

#### list_array / big_list_array
**Header:** `sparrow/list_array.hpp`

**Description:** Stores variable-length lists where each element can contain a different number of sub-elements.

**When to use:** Use the List layout when your data consists of variable-length lists and you want a straightforward, efficient representation where the order of elements in the child array matches the logical order in the parent array. This is the standard layout for most use cases involving variable-length lists, such as arrays of strings or arrays of arrays of numbers.

#### list_view_array / big_list_view_array
**Header:** `sparrow/list_array.hpp`

**Description:** Similar to list arrays but with explicit size information for each list element.

**When to use:** Use the ListView layout when you need more flexibility than the standard List layout provides, such as when the logical order of lists does not match the physical order in the child array, or when you need to represent lists that share or reuse segments of the child array. This can be useful in advanced scenarios like certain optimizations or data transformations where reordering or sharing of data is required.

#### fixed_size_list_array
**Header:** `sparrow/list_array.hpp`

**Description:** Stores lists where all elements have the same fixed number of sub-elements.

**When to use:** For homogeneous nested structures like 3D coordinates, RGB values, or fixed-size tuples.

### Struct Arrays

#### struct_array
**Header:** `sparrow/struct_array.hpp`

**Description:** Stores structured data with named fields, similar to database records or C structs.

**When to use:** For heterogeneous data with multiple named fields, like database rows or complex objects.

**Apache Arrow specification:** [Struct Layout](https://arrow.apache.org/docs/format/Columnar.html#struct-layout)

### Union Arrays

#### sparse_union_array / dense_union_array
**Header:** `sparrow/union_array.hpp`

**Description:** Stores values that can be one of several different types, similar to std::variant.

**When to use:** For polymorphic data where each element can be of different types.

**Apache Arrow specification:** [Union Layout](https://arrow.apache.org/docs/format/Columnar.html#union-layout)

### Temporal Arrays

#### timestamp_array<T>
**Header:** `sparrow/timestamp_array.hpp`

**Description:** Stores timestamps with timezone information.

**When to use:** For timestamped data with timezone awareness.

**Type aliases:**
- `timestamp_seconds_array`
- `timestamp_milliseconds_array`
- `timestamp_microseconds_array`
- `timestamp_nanoseconds_array`

#### timestamp_without_timezone_array<T>
**Header:** `sparrow/timestamp_without_timezone_array.hpp`

**Description:** Stores timestamps without timezone information.

**When to use:** For local timestamps or when timezone is not relevant.

**Type aliases:**
- `timestamp_without_timezone_seconds_array`
- `timestamp_without_timezone_milliseconds_array`
- `timestamp_without_timezone_microseconds_array`
- `timestamp_without_timezone_nanoseconds_array`

#### date_array<T>
**Header:** `sparrow/date_array.hpp`

**Description:** Stores date values (days or milliseconds since epoch).

**When to use:** For date-only data without time components.

**Type aliases:**
- `date_days_array`
- `date_milliseconds_array`

#### time_array<T>
**Header:** `sparrow/time_array.hpp`

**Description:** Stores time-of-day values with various precisions.

**When to use:** For time-only data without date components.

**Type aliases:**
- `time_seconds_array`
- `time_milliseconds_array`
- `time_microseconds_array`
- `time_nanoseconds_array`

#### duration_array<T>
**Header:** `sparrow/duration_array.hpp`

**Description:** Stores duration/time interval values.

**When to use:** For representing time spans or intervals.

**Type aliases:**
- `duration_seconds_array`
- `duration_milliseconds_array`
- `duration_microseconds_array`
- `duration_nanoseconds_array`

#### interval_array<T>
**Header:** `sparrow/interval_array.hpp`

**Description:** Stores calendar intervals (months, days, nanoseconds).

**When to use:** For complex calendar-based intervals.

**Type aliases:**
- `months_interval_array`
- `days_time_interval_array`
- `month_day_nanoseconds_interval_array`

### Decimal Arrays

#### decimal_array<T>
**Header:** `sparrow/decimal_array.hpp`

**Description:** Stores fixed-precision decimal numbers.

**When to use:** For financial calculations or when exact decimal precision is required.

### Special Layout Arrays

#### null_array
**Header:** `sparrow/null_array.hpp`

**Description:** An array where all elements are null.

**When to use:** For representing completely null columns or placeholders.

**Apache Arrow specification:** [Null Layout](https://arrow.apache.org/docs/format/Columnar.html#null-layout)

#### dictionary_encoded_array
**Header:** `sparrow/dictionary_encoded_array.hpp`

**Description:** Stores data using dictionary encoding to reduce memory usage for repeated values.

**When to use:** When data has many repeated values (categorical data, strings with duplicates).

**Apache Arrow specification:** [Dictionary Encoding](https://arrow.apache.org/docs/format/Columnar.html#dictionary-encoded-layout)

#### run_end_encoded_array
**Header:** `sparrow/run_end_encoded_array.hpp`

**Description:** Compresses data by storing run lengths for consecutive identical values.

**When to use:** For data with long runs of consecutive identical values.

**Apache Arrow specification:** [Run-end Encoding](https://arrow.apache.org/docs/format/Columnar.html#run-end-encoded-layout)

## Construction Patterns

### Using the Builder API
The most convenient way to construct arrays is using the generic builder:

```cpp
#include "sparrow/builder/builder.hpp"

// Automatically detects appropriate layout
auto arr1 = sparrow::build({1, 2, 3, 4});                    // primitive_array<int>
auto arr2 = sparrow::build({"hello", "world"});              // string_array
auto arr3 = sparrow::build({{1, 2}, {3, 4, 5}});           // list_array
auto arr4 = sparrow::build({std::make_tuple(1, "a", 2.0f)}); // struct_array
```

### Direct Construction
Each array type can be constructed directly:

```cpp
#include "sparrow/layout/primitive_layout/primitive_array.hpp"

// Direct construction with data
std::vector<int> data{1, 2, 3};
primitive_array<int> arr(data);

// With validity bitmap
std::vector<bool> validity{true, false, true};
primitive_array<int> arr2(data, validity);
```

### From Arrow C Structures
All arrays can be constructed from Arrow C interface structures:

```cpp
// From ArrowArray and ArrowSchema
primitive_array<int> arr(arrow_proxy(std::move(arrow_array), std::move(arrow_schema)));
```
