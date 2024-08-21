#include "doctest/doctest.h"

namespace  sparrow::test
{  


    template<class Layout>
    void layout_tester_impl(Layout && layout)
    {
        using layout_type = std::decay_t<Layout>;
        using size_type = typename layout_type::size_type;
        const auto size = layout.size();

        auto iter_begin = layout.begin();
        auto iter_end = layout.end();
        auto iter_cbegin = layout.cbegin();
        auto iter_cend = layout.cend();
        auto values_range = layout.values();
        auto bitmap_range = layout.bitmap();


        using value_range_diff_type = std::ranges::range_difference_t<decltype(values_range)>;
        using bitmap_range_diff_type = std::ranges::range_difference_t<decltype(bitmap_range)>;

        using iter_diff_type = std::iterator_traits<decltype(iter_begin)>::difference_type;

        // check for consitent sizes
        SUBCASE("sizes"){
            CHECK(size == values_range.size());
            CHECK(size == bitmap_range.size());
            CHECK(size == std::distance(iter_begin, iter_end));
            CHECK(size == std::distance(iter_cbegin, iter_cend));
        }
        
        for( size_type i = 0; i < size; ++i){
            // check for consistent values
            auto maybe_value = layout[i];
            if(maybe_value.has_value()){
                auto value = maybe_value.value();
                SUBCASE("has_value"){
                    CHECK(iter_begin[iter_diff_type(i)].has_value());
                    CHECK(iter_cbegin[iter_diff_type(i)].has_value());
                    CHECK(bool(bitmap_range[bitmap_range_diff_type(i)]));
                }
                SUBCASE("values"){
                    CHECK(value == values_range[value_range_diff_type(i)]);
                    CHECK(value == iter_begin[iter_diff_type(i)].value());
                    CHECK(value == iter_cbegin[iter_diff_type(i)].value());
                }
            }
            else{
                SUBCASE("has_value"){
                    CHECK_FALSE(iter_begin[iter_diff_type(i)].has_value());
                    CHECK_FALSE(iter_cbegin[iter_diff_type(i)].has_value());
                }
            }
            // check for consistent bitmaps
            SUBCASE("bitmaps"){
                CHECK(maybe_value.has_value() == bitmap_range[bitmap_range_diff_type(i)]);
            }
        }
    }



    template<class Layout>
    void layout_tester(Layout && layout)
    {
        SUBCASE("non-const"){
            layout_tester_impl(std::forward<Layout>(layout));
        }
        SUBCASE("const"){
            using layout_type = std::decay_t<Layout>;
            const layout_type & const_layout = layout;
            layout_tester_impl(const_layout);
        }
    }

    
} // namespace  sparrow
