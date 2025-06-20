# Sparrow Array Layouts

This document provides a comprehensive overview of all available array layouts in Sparrow, including their header paths, use cases, and construction methods.

## Primitive Arrays

### primitive_array<T>
**Header:** `sparrow/layout/primitive_layout/primitive_array.hpp`

**Description:** Stores scalar values of a specific type (integers, floats, booleans) in a contiguous memory layout.
T type can be:
- `int8_t`, `int16_t`, `int32_t`, `int64_t`
- `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`
- `float16_t`, `float`, `double`
- `bool`, `std::byte`

**When to use:** For homogeneous collections of scalar values.

## Variable-Size Binary Arrays

### string_array / big_string_array
**Header:** `sparrow/layout/variable_size_binary_layout/variable_size_binary_array.hpp`

**Description:** Stores variable-length strings or binary data. `big_string_array` uses 64-bit offsets for larger datasets.

**When to use:** For text data, variable-length strings, or binary blobs.

### binary_array / big_binary_array
**Header:** `sparrow/layout/variable_size_binary_layout/variable_size_binary_array.hpp`

**Description:** Similar to string arrays but for binary data (sequences of bytes).

**When to use:** For variable-length binary data like images, serialized objects, or raw byte sequences.

## Fixed-Width Binary Arrays

### fixed_width_binary_array
**Header:** `sparrow/layout/fixed_width_binary_array.hpp`

**Description:** Stores fixed-size binary data where all elements have the same byte length.

**When to use:** For fixed-size binary data like UUIDs, hashes, or fixed-size encoded data.

## List Arrays

### list_array / big_list_array
**Header:** `sparrow/layout/list_layout/list_array.hpp`

**Description:** Stores variable-length lists where each element can contain a different number of sub-elements.

**When to use:** For nested data structures like arrays of arrays, where inner arrays can have different lengths.

### list_view_array / big_list_view_array
**Header:** `sparrow/layout/list_layout/list_array.hpp`

**Description:** Similar to list arrays but with explicit size information for each list element.

**When to use:** When you need explicit control over list sizes or when working with overlapping list views.

### fixed_sized_list_array
**Header:** `sparrow/layout/list_layout/list_array.hpp`

**Description:** Stores lists where all elements have the same fixed number of sub-elements.

**When to use:** For homogeneous nested structures like 3D coordinates, RGB values, or fixed-size tuples.

## Struct Arrays

### struct_array
**Header:** `sparrow/layout/struct_layout/struct_array.hpp`

**Description:** Stores structured data with named fields, similar to database records or C structs.

**When to use:** For heterogeneous data with multiple named fields, like database rows or complex objects.

## Union Arrays

### sparse_union_array / dense_union_array
**Header:** `sparrow/layout/union_array.hpp`

**Description:** Stores values that can be one of several different types, similar to std::variant.

**When to use:** For polymorphic data where each element can be of different types.

## Temporal Arrays

### timestamp_array<T>
**Header:** `sparrow/layout/temporal/timestamp_array.hpp`

**Description:** Stores timestamps with timezone information.

**When to use:** For timestamped data with timezone awareness.

**Type aliases:**
- `timestamp_seconds_array`
- `timestamp_milliseconds_array`
- `timestamp_microseconds_array`
- `timestamp_nanoseconds_array`

### timestamp_without_timezone_array<T>
**Header:** `sparrow/layout/temporal/timestamp_without_timezone_array.hpp`

**Description:** Stores timestamps without timezone information.

**When to use:** For local timestamps or when timezone is not relevant.

**Type aliases:**
- `timestamp_without_timezone_seconds_array`
- `timestamp_without_timezone_milliseconds_array`
- `timestamp_without_timezone_microseconds_array`
- `timestamp_without_timezone_nanoseconds_array`

### date_array<T>
**Header:** `sparrow/layout/temporal/date_array.hpp`

**Description:** Stores date values (days or milliseconds since epoch).

**When to use:** For date-only data without time components.

**Type aliases:**
- `date_days_array`
- `date_milliseconds_array`

### time_array<T>
**Header:** `sparrow/layout/temporal/time_array.hpp`

**Description:** Stores time-of-day values with various precisions.

**When to use:** For time-only data without date components.

**Type aliases:**
- `time_seconds_array`
- `time_milliseconds_array`
- `time_microseconds_array`
- `time_nanoseconds_array`

### duration_array<T>
**Header:** `sparrow/layout/temporal/duration_array.hpp`

**Description:** Stores duration/time interval values.

**When to use:** For representing time spans or intervals.

**Type aliases:**
- `duration_seconds_array`
- `duration_milliseconds_array`
- `duration_microseconds_array`
- `duration_nanoseconds_array`

### interval_array<T>
**Header:** `sparrow/layout/temporal/interval_array.hpp`

**Description:** Stores calendar intervals (months, days, nanoseconds).

**When to use:** For complex calendar-based intervals.

**Type aliases:**
- `months_interval_array`
- `days_time_interval_array`
- `month_day_nanoseconds_interval_array`

## Decimal Arrays

### decimal_array<T>
**Header:** `sparrow/layout/decimal_array.hpp`

**Description:** Stores fixed-precision decimal numbers.

**When to use:** For financial calculations or when exact decimal precision is required.

## Special Layout Arrays

### null_array
**Header:** `sparrow/layout/null_array.hpp`

**Description:** An array where all elements are null.

**When to use:** For representing completely null columns or placeholders.

**Apache Arrow specification:** [Null Layout](https://arrow.apache.org/docs/format/Columnar.html#null-layout)

### dictionary_encoded_array
**Header:** `sparrow/layout/dictionary_encoded_array.hpp`

**Description:** Stores data using dictionary encoding to reduce memory usage for repeated values.

**When to use:** When data has many repeated values (categorical data, strings with duplicates).

**Apache Arrow specification:** [Dictionary Encoding](https://arrow.apache.org/docs/format/Columnar.html#dictionary-encoded-layout)

### run_end_encoded_array
**Header:** `sparrow/layout/run_end_encoded_layout/run_end_encoded_array.hpp`

**Description:** Compresses data by storing run lengths for consecutive identical values.

**When to use:** For data with long runs of identical values.

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
