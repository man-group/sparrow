/** @example typed_array_low_level.cpp
 * Create a typed_array object from an array_data object.
 * The array_data object is created with 5 float32 elements where the value at index 2 is missing.
 * The typed_array is created from the array_data and the data is accessed.
 */

#include <sparrow/array/array_data.hpp>
#include <sparrow/array/array_data_factory.hpp>
#include <sparrow/array.hpp>

#include <sparrow/layout/list_layout.hpp>

#include <vector>
#include <iostream>






int main() {

    // raw data
    std::vector<std::vector<int>> values = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}};
    // flatter by hand
    std::vector<int> flat_values;
    for(auto & v : values){
        for(auto & e : v){
            flat_values.push_back(e);
        }
    }


    using data_storage = sparrow::array_data;

    // layout type of the inner flat array
    using inner_layout_type = sparrow::fixed_size_layout<int, data_storage>;

    // inner list as array_data
    auto values_array_data = sparrow::make_default_array_data<inner_layout_type>(flat_values);

    // inner layout (not needed to build the list)
    inner_layout_type inner_layout(values_array_data);

    for(auto i = 0; i < inner_layout.size(); ++i){
        std::cout << inner_layout.begin()[i].value() << "\n";
    }


    auto list_array_data = sparrow::array_data{};

    // create the buffer for the offsets of the outer list
    sparrow::array_data::buffer_type offset_buffer(sizeof(std::int64_t) * (values.size()  + 1),0);
    auto offset_buffer_ptr = offset_buffer.data<std::int64_t>();

    // fill the buffer with the offsets
    std::int64_t offset = 0;
    offset_buffer_ptr[0] = 0;
    for(auto i = 0; i < values.size(); ++i){
        offset_buffer_ptr[i+1] = offset_buffer_ptr[i] + values[i].size();
    }

    // print all offsets
    std::cout<<"offsets:\n";
    for(auto i = 0; i < values.size() + 1; ++i){
        std::cout <<i<<" "<<offset_buffer_ptr[i] << "\n";
    }
    


    list_array_data.buffers.push_back(std::move(offset_buffer));
    list_array_data.child_data.push_back(std::move(values_array_data));


    // create the bitmap for the outer list
    sparrow::dynamic_bitset<uint8_t> bitmap(values.size(), true);
    list_array_data.bitmap = std::move(bitmap);
    list_array_data.length = values.size();

    using list_layout_type = sparrow::list_layout<inner_layout_type, data_storage,  std::int64_t>;

    list_layout_type list_layout(list_array_data);


    for(auto maybe_list : list_layout){
        if(maybe_list.has_value()){
            auto list = maybe_list.value();
            std::cout<<"size: "<<list.size()<<"\n";
            for(auto value : list){
                if(value.has_value()){
                    std::cout << value.value() << " ";
                }
                else{
                    std::cout << "missing value ";
                }
            }
            std::cout << "\n";
        }
        else{
            std::cout << "missing value\n";
        }

    }

}
