/** @example typed_array.cpp
 * Create a typed_array object from an array_data object.
 * The array_data object is created with 5 float32 elements where the value at index 2 is missing.
 * The typed_array is created from the array_data and the data is accessed.
 */

#include <sparrow/array/array_data.hpp>
#include <sparrow/array.hpp>

int main() {
    /////////////////////////////////////////////////
    // create array_data with 5 float32 elements
    // where the value at index 2 is missing
    /////////////////////////////////////////////////
    /*using value_type = float;
    auto n = 5;

    // create the array_data object 
    sparrow::array_data ad;
    ad.type = sparrow::data_descriptor(sparrow::arrow_traits<value_type>::type_id);

    // create a bitmap with all bits set to true exceot for 
    // the missing value at index 2
    ad.bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);
    ad.bitmap.set(2, false);


    //  the buffer holding the actual data
    const size_t buffer_size = (n * sizeof(value_type)) / sizeof(uint8_t);
    sparrow::buffer<uint8_t> b(buffer_size);

    b.data<value_type>()[0] = 1.0;
    b.data<value_type>()[1] = 2.0;
    // b.data<value_type>()[2] = ???; // missing value
    b.data<value_type>()[3] = 4.0;
    b.data<value_type>()[4] = 5.0;

    // add the buffer to the array_data
    ad.buffers.push_back(b);
    ad.length = static_cast<std::int64_t>(n);
    ad.offset = static_cast<std::int64_t>(0);
    ad.child_data.emplace_back();
    
    // create a typed_array object from the array_data
    auto array = sparrow::typed_array<value_type>(ad);

    // access the data
    for (auto i = 0; i < n; ++i) {
        if (array.bitmap()[i]) {
            std::cout << array[i].value() << std::endl;
        } else {
            std::cout << "missing value" << std::endl;
        }
    }*/

    return 0;
}
