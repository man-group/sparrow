// Unit tests for memory_alignment.hpp
#include "sparrow/utils/memory_alignment.hpp"

#include "doctest/doctest.h"

namespace sparrow
{
    TEST_SUITE("memory_alignment")
    {
        TEST_CASE("align_to_64_bytes")
        {
            CHECK_EQ(align_to_64_bytes(0), 0);
            CHECK_EQ(align_to_64_bytes(1), 64);
            CHECK_EQ(align_to_64_bytes(63), 64);
            CHECK_EQ(align_to_64_bytes(64), 64);
            CHECK_EQ(align_to_64_bytes(65), 128);
            CHECK_EQ(align_to_64_bytes(128), 128);
            CHECK_EQ(align_to_64_bytes(129), 192);
        }

        TEST_CASE("calculate_aligned_size")
        {
            // For int (usually 4 bytes)
            CHECK_EQ(calculate_aligned_size<int>(0), 0);
            CHECK_EQ(calculate_aligned_size<int>(1), 64);
            CHECK_EQ(calculate_aligned_size<int>(16), 64);   // 16*4=64
            CHECK_EQ(calculate_aligned_size<int>(17), 128);  // 17*4=68 -> 128
            // For char (1 byte)
            CHECK_EQ(calculate_aligned_size<char>(1), 64);
            CHECK_EQ(calculate_aligned_size<char>(64), 64);
            CHECK_EQ(calculate_aligned_size<char>(65), 128);
            // For double (8 bytes)
            CHECK_EQ(calculate_aligned_size<double>(8), 64);   // 8*8=64
            CHECK_EQ(calculate_aligned_size<double>(9), 128);  // 9*8=72 -> 128
        }
    }

}  // namespace sparrow
