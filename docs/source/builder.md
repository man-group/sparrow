# Builder                       {#builder}

## Motivation


Arrow data structures are "structs of arrays" (SoA) which are all composed
of flat arrays. This is a very efficient way to store data, but might be cumbersome
to create such data structures.
To illustate this, consider the following example. Suppose you have a list of
lists of integers, like `[[1, 2], [3, 4, 5], [6, 7]]`. 
The usual way of storing (and thinking about) this data is as a vector of vectors
`std::vector<std::vector<int>>`. 
In Arrow format, this data is stored as two flat arrays (ingoring the null bitmap for now):

- A flat array of integers `[1, 2, 3, 4, 5, 6, 7]`
- An array of offsets `[0, 2, 5, 7]` which indicates the start of each list.

The last element of the offsets array is the total number of elements in the flat array.
Therfore, to build the arrow data structure, we need to think in terms of the flattened array.
While this is still relatively simple for the example above, it can become quite complex
once we have even more nested data structures.
For a tripple nested list of integers `[[[1, 2], [3, 4]], [[5, 6], [7, 8]]]`, we would need to
create the following arrays:

- A flat array of integers `[1, 2, 3, 4, 5, 6, 7, 8]`
- An array of offsets `[0, 2, 4, 8]` which indicates the start of the inner lists.
- An array of offsets `[0, 2, 4]` which indicates the start of the outer lists.

This is where the `builder` comes in. It provides a convenient way to build Arrow data structures
from nested stl containers, ie from things like `std::vector<std::vector<int>>`, `std::vector<std::tuple<int, double>>`, etc.

## Type / Layout Mapping 

The `sparrow::builder` function is a template function that takes arbitrary nested stl containers
and returns an Arrow data structure.
The mapping between stl containers and Arrow data structures is conceptually the following:





| Concept | Arrow Layout | Example |
|  ---------------------------- | ----------------------------- | ------------------------------------------|
| range of scalars              | primitive layout              | `std::vector<int>`                        |
| range of string               | variable sized binary layout  | `std::vector<std::string>`                |
| range of range                | list layout                   | `std::vector<std::vector<int>>`           |
| range of fixed-size range     | fixed size list layout        | `std::vector<std::array<int,3>>`          |
| range of tuples               | struct layout                 | `std::vector<std::tuple<int, double>>`    |
| range of variants             | sparse/dense union layout     | `std::vector<std::variant<int, double>>`  |




When nesting containers, the above rules are applied recursively. For example, a `std::vector<std::vector<int>>` would be converted to a list layout of primitive layout (ie `list[int]`).
Here are some more examples:


|Type                                                   | Arrow Layout                  |
|-------------------------------------------------------|-------------------------------|
|std::vector<std::vector<int>>                          | list[int]                     |
|std::vector<std::vector<std::string>>                  | list[utf8]                    |
|std::vector<std::vector<std::tuple<int, double>>>      | list[struct[int, double]]     |
|std::vector<std::vector<std::variant<int, double>>>    | list[variant[int, double]]    |
|std::vector<std::vector<std::array<int, 3>>>           | list[fixed_size_list[int]]    |
|std::vector<std::tuple<std::vector<int>, std::string>> | struct[list[int], utf8]       |


## Null Support

Arrow has support for null values. This means an array element at a certain index can be missing.
The `sparrow::builder` function supports this by using `sparrow::nullable` (simmilar to `std::optional`).
Nullables can be "injected" into all levels of the nested stl containers.
For example, a `std::vector<sparrow::nullable<int>>{ 1, 2, sp::null, 4}` would be converted to a primitive layout with where the third element is missing. `std::vector<std::tuple<int, sp::nullable<double>>>{ {1, 2.0}, {3, sp::null}, {4, 5.0} }` would be converted to a struct layout with the second element of the second tuple missing.

The union layout in sparrow does not have its own bitmap. Instead the bitmap of the elements in the union is used. Therefore, for having nullable values with unions, the nullable must be injected into the union elements:
ie `std::vector<std::variant<sp::nullable<int>, sp::nullable<double>>>` instead of `std::vector<sp::nullable<std::variant<int, double>>>`.

## Dict Encoding and Run End Encoding


Arrow has two special encodings: dict encoding and run end encoding.
Dict encoding is used for columns with a small number of unique values. Instead of storing the values themselves, the indices of the values in a dictionary are stored.
Run end encoding is used for columns with long runs of the same value. Instead of storing the value for each element, the value is stored once and the number of times it is repeated is stored.

Since **any** any array of arbitrary layout can be "dict-encoded" or "run-end-encoded", it is ambiguous how to handle these encodings in the builder. A `std::vector<int>` could be mapped to a primitive layout, a dict-encoded primitive layout or a run-end-encoded primitive layout.

`sparrow::dict_encode<std::vector<int>>` will be a dict-encoded primitive layout, `sparrow::run_end_encode<std::vector<int>>` will be a run-end-encoded primitive layout.

The dict-encoding and run-end-encoding can also appear **inside** the nested stl containers. For example, `std::vector<std::tuple<int, sparrow::dict_encode<std::string>>>` would be a struct layout with the second element beeing a dict encoded variable sized binary layout.

Note that the builder does not support consecutive nesting of dict_encoding and run_end_encoding.
Ie `sparrow::dict_encode<sparrow::dict_encode<std::vector<int>>>` is not supported.
Simmilar `sparrow::dict_encode<sparrow::run_end_encode<std::vector<int>>>` is not supported.


## Usage Example

### Primitive Array

#### Primitive Array of Integers
@snippet{trimleft} examples/builder_example.cpp builder_primitive_array

#### Primitive Array of Integers with Nulls
@snippet{trimleft} examples/builder_example.cpp builder_primitive_array_with_nulls

### List Array

#### ListArray of String
@snippet{trimleft} examples/builder_example.cpp builder_list_of_strings

#### ListArray of Strings with Nulls
@snippet{trimleft} examples/builder_example.cpp builder_list_of_strings_with_nulls

### ListArray of Struct Array
@snippet{trimleft} examples/builder_example.cpp builder_list_of_struct

## Fixed-Sized List

### Fixed-Sized List Array of Variable-Sized Binary A rray
@snippet{trimleft} examples/builder_example.cpp builder_fixed_sized_list_strings

### Fixed-Sized List Array of Union Array
@snippet{trimleft} examples/builder_example.cpp builder_fixed_sized_list_of_union

## Struct Array

@snippet{trimleft} examples/builder_example.cpp builder_struct_array

## Union Array

@snippet{trimleft} examples/builder_example.cpp builder_union_array

## Dict Encoded Array

### Dict Encoded Variable-Sized Binary Array
@snippet{trimleft} examples/builder_example.cpp builder_dict_encoded_variable_sized_binary

## Run End Encoded Array
@snippet{trimleft} examples/builder_example.cpp builder_run_end_encoded_variable_sized_binary

