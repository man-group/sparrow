#include "sparrow/typed_array.hpp"

template <typename T>
    sparrow::array_data
    make_test_array_data(size_t n = 10, size_t offset = 0, const std::vector<size_t>& false_bitmap = {})
    {
        sparrow::array_data ad;
        ad.type = sparrow::data_descriptor(sparrow::arrow_traits<T>::type_id);
        ad.bitmap = sparrow::dynamic_bitset<uint8_t>(n, true);
        for (const auto i : false_bitmap)
        {
            if (i >= n)
            {
                throw std::invalid_argument("Index out of range");
            }
            ad.bitmap.set(i, false);
        }
        const size_t buffer_size = (n * sizeof(T)) / sizeof(uint8_t);
        sparrow::buffer<uint8_t> b(buffer_size);
        for (uint8_t i = 0; i < n; ++i)
        {
            b.data<T>()[i] = static_cast<T>(i);
        }
        ad.buffers.push_back(b);
        ad.length = n;
        ad.offset = offset;
        ad.child_data.emplace_back();
        return ad;
    }

int main() {
    const auto array_data = make_test_array_data<int>(10, 0);
    const sparrow::typed_array<int> ta{array_data};
    const sparrow::typed_array<int> ta_same{array_data};
    if(ta == ta)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}