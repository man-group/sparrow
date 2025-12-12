# Record Batch      {#record_batch}

## Overview

The `record_batch` class provides a table-like data structure for storing columnar data with named fields. It represents a collection of equal-length arrays mapped to unique column names, similar to a database table or DataFrame where each array forms a column.

This class is particularly useful for:
- Working with tabular data in Arrow format
- Interoperating with Arrow C structures
- Batch processing of columnar data
- Serialization and data exchange

## Key Features

- **Column-oriented storage**: Data is stored as arrays with associated names
- **Arrow compatibility**: Direct integration with Arrow C structures (ArrowArray and ArrowSchema)
- **Type safety**: Built on top of the `struct_array` class
- **Efficient access**: O(1) column access by index, O(1) average access by name
- **Validation**: Ensures all arrays have the same length and column names are unique
- **Flexible construction**: Multiple ways to create record batches

## Basic Usage

### Creating a Record Batch

#### From Separate Names and Arrays

```cpp
#include "sparrow/record_batch.hpp"
#include "sparrow/primitive_array.hpp"

// Create arrays
sparrow::primitive_array<int32_t> id_array({1, 2, 3, 4, 5});
sparrow::primitive_array<std::string> name_array({"Alice", "Bob", "Charlie", "David", "Eve"});
sparrow::primitive_array<int32_t> age_array({25, 30, 35, 40, 45});

// Create column names
std::vector<std::string> names = {"id", "name", "age"};

// Create arrays list
std::vector<sparrow::array> columns = {
    sparrow::array(std::move(id_array)),
    sparrow::array(std::move(name_array)),
    sparrow::array(std::move(age_array))
};

// Create record batch
sparrow::record_batch batch(names, columns, "employee_data");
```

#### From Named Arrays

```cpp
// Create arrays with names
auto iota = std::ranges::iota_view{0, 10};
sparrow::primitive_array<uint16_t> pr0(
    iota | std::views::transform([](auto i) { return static_cast<uint16_t>(i); }),
    true,  // owning
    "first"  // column name
);

auto iota2 = std::ranges::iota_view{4, 14};
sparrow::primitive_array<int32_t> pr1(iota2, true, "second");

auto iota3 = std::ranges::iota_view{2, 12};
sparrow::primitive_array<int32_t> pr2(iota3, true, "third");

// Create record batch from named arrays
std::vector<sparrow::array> named_columns = {
    sparrow::array(std::move(pr0)),
    sparrow::array(std::move(pr1)),
    sparrow::array(std::move(pr2))
};

sparrow::record_batch batch(named_columns, "my_batch");
```

#### From Initializer List

```cpp
sparrow::record_batch batch = {
    {"id", id_array},
    {"name", name_array},
    {"age", age_array}
};
```

#### From Struct Array

```cpp
sparrow::struct_array arr(columns);
sparrow::record_batch batch(std::move(arr));
```

### Accessing Data

#### Getting Column Information

```cpp
// Get number of columns and rows
std::size_t num_cols = batch.nb_columns();  // Returns 3
std::size_t num_rows = batch.nb_rows();     // Returns 5

// Check if a column exists
bool has_age = batch.contains_column("age");  // Returns true
bool has_salary = batch.contains_column("salary");  // Returns false

// Get column name by index
const std::string& col_name = batch.get_column_name(0);  // Returns "id"

// Get record batch name
const auto& batch_name = batch.name();  // Returns std::optional<std::string>
```

#### Accessing Columns

```cpp
// Get column by name
const sparrow::array& age_col = batch.get_column("age");

// Get column by index
const sparrow::array& first_col = batch.get_column(0);

// Get all column names
auto names = batch.names();  // Returns a range of column names
for (const auto& name : names) {
    std::cout << name << std::endl;
}

// Get all columns
auto columns = batch.columns();  // Returns a range of arrays
for (const auto& col : columns) {
    // Process each column
}
```

#### Mutable Access

```cpp
// Get mutable reference to column
sparrow::array& col = batch.get_column("age");

// Modify column (if applicable)
// Note: The array itself determines if modifications are allowed
```

### Modifying a Record Batch

#### Adding Columns

```cpp
// Add column with explicit name
sparrow::primitive_array<double> salary_array({50000.0, 60000.0, 70000.0, 80000.0, 90000.0});
batch.add_column("salary", sparrow::array(std::move(salary_array)));

// Add column using array's internal name
sparrow::primitive_array<std::string> dept_array(
    {"HR", "IT", "Sales", "IT", "HR"},
    true,
    "department"
);
batch.add_column(sparrow::array(std::move(dept_array)));
```

### Extracting Struct Array

```cpp
// Extract the underlying struct array
sparrow::struct_array arr = batch.extract_struct_array();
// Note: After extraction, the record batch is in a moved-from state
```

## Arrow C Interface Integration

The `record_batch` class provides seamless integration with Arrow C structures.

### From Arrow C Structures

#### Taking Ownership

```cpp
// Transfer ownership of both ArrowArray and ArrowSchema
ArrowArray arr = /* ... */;
ArrowSchema sch = /* ... */;
sparrow::record_batch batch(std::move(arr), std::move(sch));
```

#### Taking Ownership of Array Only

```cpp
// Transfer ownership of ArrowArray, reference ArrowSchema
ArrowArray arr = /* ... */;
ArrowSchema* sch = /* ... */;
sparrow::record_batch batch(std::move(arr), sch);
```

#### Referencing Existing Structures

```cpp
// Reference both structures (no ownership transfer)
ArrowArray* arr = /* ... */;
ArrowSchema* sch = /* ... */;
sparrow::record_batch batch(arr, sch);

// Works with const pointers too
const ArrowArray* const_arr = /* ... */;
const ArrowSchema* const_sch = /* ... */;
sparrow::record_batch batch2(const_arr, const_sch);
```

## Comparison and Equality

```cpp
sparrow::record_batch batch1(names1, columns1);
sparrow::record_batch batch2(names2, columns2);

// Compare for equality
if (batch1 == batch2) {
    std::cout << "Batches are equal" << std::endl;
}

// Check inequality
if (batch1 != batch2) {
    std::cout << "Batches are different" << std::endl;
}
```

Two record batches are equal if:
- They have the same number of columns
- Column names match in the same order
- Corresponding arrays are equal
- Record batch names match (both present and equal, or both absent)

## Formatting and Printing

If C++20 `<format>` is available, `record_batch` supports formatting:

```cpp
#if defined(__cpp_lib_format)
const sparrow::record_batch batch = /* ... */;
std::string formatted = std::format("{}", batch);
std::cout << formatted << std::endl;

// Output example:
// |first|second|third|
// --------------------
// |    0|     4|    2|
// |    1|     5|    3|
// |    2|     6|    4|
// ...
#endif
```

Stream output is also supported:

```cpp
std::cout << batch << std::endl;
```

## Copy and Move Semantics

### Copy Operations

```cpp
// Copy constructor
sparrow::record_batch batch1 = /* ... */;
sparrow::record_batch batch2(batch1);  // Deep copy

// Copy assignment
sparrow::record_batch batch3 = /* ... */;
batch3 = batch1;  // Deep copy
```

### Move Operations

```cpp
// Move constructor
sparrow::record_batch batch1 = /* ... */;
sparrow::record_batch batch2(std::move(batch1));  // batch1 is now invalid

// Move assignment
sparrow::record_batch batch3 = /* ... */;
batch3 = std::move(batch2);  // batch2 is now invalid
```

## Constraints and Invariants

### Preconditions

- **Equal array lengths**: All arrays must have the same length
- **Unique column names**: Column names must be unique within a record batch
- **Non-empty names**: When constructing from named arrays, each array must have a non-empty name
- **Size matching**: When providing separate names and arrays, both ranges must have the same size

### Postconditions

- **Consistent state**: The record batch maintains internal consistency between names, arrays, and the name-to-array mapping
- **O(1) access**: Column access by index is O(1), by name is O(1) average time
- **Thread safety**: Read operations are thread-safe; write operations require external synchronization

## Performance Considerations

- **Name lookup**: Uses an internal hash map for O(1) average-case column lookup by name
- **Lazy map construction**: The name-to-array map is built lazily and cached
- **Copy costs**: Copying a record batch performs a deep copy of all arrays
- **Move efficiency**: Move operations are efficient and don't copy array data

## Complete Example

```cpp
#include <cassert>
#include "sparrow/primitive_array.hpp"
#include "sparrow/record_batch.hpp"

std::vector<sparrow::array> make_array_list(const std::size_t data_size)
{
    auto iota = std::ranges::iota_view{std::size_t(0), std::size_t(data_size)};
    sparrow::primitive_array<std::uint16_t> pr0(
        iota | std::views::transform([](auto i) {
            return static_cast<std::uint16_t>(i);
        })
    );
    
    auto iota2 = std::ranges::iota_view{std::int32_t(4), 4 + std::int32_t(data_size)};
    sparrow::primitive_array<std::int32_t> pr1(iota2);
    
    auto iota3 = std::ranges::iota_view{std::int32_t(2), 2 + std::int32_t(data_size)};
    sparrow::primitive_array<std::int32_t> pr2(iota3);

    return {
        sparrow::array{std::move(pr0)},
        sparrow::array{std::move(pr1)},
        sparrow::array{std::move(pr2)}
    };
}

int main()
{
    const std::vector<std::string> name_list = {"first", "second", "third"};
    constexpr std::size_t data_size = 10;
    const std::vector<sparrow::array> array_list = make_array_list(data_size);
    
    // Create record batch
    const sparrow::record_batch record{name_list, array_list, "record batch name"};
    
    // Verify properties
    assert(record.name() == "record batch name");
    assert(record.nb_columns() == array_list.size());
    assert(record.nb_rows() == data_size);
    assert(record.contains_column(name_list[0]));
    assert(record.get_column_name(0) == name_list[0]);
    assert(record.get_column(0) == array_list[0]);
    assert(std::ranges::equal(record.names(), name_list));
    assert(std::ranges::equal(record.columns(), array_list));
    
    return EXIT_SUCCESS;
}
```

## See Also

- [struct_array](typed_array.md#struct-arrays) - The underlying array type for record batches
- [array](array.md) - The type-erased array wrapper
- [Arrow C Data Interface](https://arrow.apache.org/docs/format/CDataInterface.html) - Arrow C structure specification
