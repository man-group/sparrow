/** @example typed_array.cpp
 * Create a typed_array object from an array_data object.
 * The array_data object is created with 5 float32 elements where the value at index 2 is missing.
 * The typed_array is created from the array_data and the data is accessed.
 */

#include <sparrow/array/array_data.hpp>
#include <sparrow/array.hpp>




/////////////////////////////////////////////////
// create array_data with 5 float32 elements
/////////////////////////////////////////////////
void example_float_typed_array(){
    using value_type = float;
    std::vector<value_type> data = {1.0, 2.0, 3.0, 4.0, 5.0};
    
    // create a typed_array object from the array_data

    auto array = sparrow::typed_array<value_type>(data);

    // access the data
    for (auto i = 0; i < array.size(); ++i) {

        if (array.bitmap()[i]) {
            std::cout << array[i].value() << std::endl;
        } else {
           std::cout << "missing value" << std::endl;
        }
    }
}



void example_string_typed_array(){
    using value_type = std::string;
    std::vector<std::string> data = {
        "one",
        "two",
        "three",
        "four",
        "five"
    };
    
    // create a typed_array object from the array_data

    auto array = sparrow::typed_array<value_type>(data);
    const auto size = array.size();
    // access the data
    for (auto i = 0; i <size; ++i) {
        if (array.bitmap()[i]) {
            auto  value = array[i].value();
            std::cout << std::string(value.begin(), value.end()) << std::endl;
        } else {
        }
    }
}







int main() {
    example_float_typed_array();
    example_string_typed_array();

    return 0;
}
