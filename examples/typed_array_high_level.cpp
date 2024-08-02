/** @example typed_array.cpp
 * Create a typed_array object from a range.
 * The range can be a range of values or a range of nullable values.
 */

#include <sparrow/array/array_data.hpp>
#include <sparrow/array.hpp>


/*void example_typed_array_of_floats(){
    using value_type = float;
    std::vector<value_type> data = {1.0, 2.0, 3.0, 4.0, 5.0};
    
    // construct the array
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

void example_typed_array_of_strings(){
    using value_type = std::string;
    std::vector<std::string> data = {
        "one",
        "two",
        "three",
        "four",
        "five"
    };
    
    // construct the array
    auto array = sparrow::typed_array<value_type>(data);

    // access the data
    for (auto i = 0; i <array.size(); ++i) {
        if (array.bitmap()[i]) {
            auto  value = array[i].value();
            std::cout << std::string(value.begin(), value.end()) << std::endl;
        } else {
            std::cout << "missing value" << std::endl;
        }
    }
}

void example_typed_array_of_strings_from_nullables(){

    using value_type = std::string;
    using nullable_type = sparrow::nullable<value_type>;

    std::vector<nullable_type> data = {
        nullable_type("one"),
        nullable_type("two"),
        nullable_type(),
        nullable_type("four"),
        nullable_type("five")
    };
    
    // construct the array
    auto array = sparrow::typed_array<value_type>(data);

    // access the data
    for (auto i = 0; i <array.size(); ++i) {
        if (array.bitmap()[i]) {
            auto  value = array[i].value();
            std::cout << std::string(value.begin(), value.end()) << std::endl;
        } else {
            std::cout << "missing value" << std::endl;
        }
    }
}*/
int main() {
    //example_typed_array_of_floats();
    //example_typed_array_of_strings();
    //example_typed_array_of_strings_from_nullables();
    return 0;
}
