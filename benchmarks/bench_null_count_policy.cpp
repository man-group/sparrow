// Copyright 2024 Man Group Operations Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <climits>
#include <cstdint>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include "sparrow/buffer/dynamic_bitset/null_count_policy.hpp"

namespace sparrow::benchmark
{
    // Constants for benchmarking
    constexpr size_t RANDOM_SEED = 42;

    // Generate random block data with specified bit density
    template <typename BlockType>
    std::vector<BlockType> generate_block_data(size_t block_count, double true_probability)
    {
        std::vector<BlockType> data(block_count);
        std::minstd_rand gen(RANDOM_SEED);

        constexpr size_t bits_per_block = sizeof(BlockType) * CHAR_BIT;
        const auto threshold = static_cast<std::minstd_rand::result_type>(
            true_probability * std::minstd_rand::max()
        );

        for (size_t i = 0; i < block_count; ++i)
        {
            BlockType block = 0;
            for (size_t bit = 0; bit < bits_per_block; ++bit)
            {
                if (gen() < threshold)
                {
                    block |= static_cast<BlockType>(BlockType{1} << bit);
                }
            }
            data[i] = block;
        }

        return data;
    }

    // Benchmark: count_non_null with 50% bit density
    template <typename BlockType>
    static void BM_CountNonNull_50Percent(::benchmark::State& state)
    {
        constexpr size_t bits_per_block = sizeof(BlockType) * CHAR_BIT;
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t block_count = (bit_size + bits_per_block - 1) / bits_per_block;

        auto data = generate_block_data<BlockType>(block_count, 0.5);
        tracking_null_count<> policy;

        for (auto _ : state)
        {
            auto count = policy.count_non_null(data.data(), bit_size, block_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * block_count * sizeof(BlockType)));
    }

    // Benchmark: count_non_null with 10% bit density (sparse)
    template <typename BlockType>
    static void BM_CountNonNull_10Percent(::benchmark::State& state)
    {
        constexpr size_t bits_per_block = sizeof(BlockType) * CHAR_BIT;
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t block_count = (bit_size + bits_per_block - 1) / bits_per_block;

        auto data = generate_block_data<BlockType>(block_count, 0.1);
        tracking_null_count<> policy;

        for (auto _ : state)
        {
            auto count = policy.count_non_null(data.data(), bit_size, block_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * block_count * sizeof(BlockType)));
    }

    // Benchmark: count_non_null with 90% bit density (dense)
    template <typename BlockType>
    static void BM_CountNonNull_90Percent(::benchmark::State& state)
    {
        constexpr size_t bits_per_block = sizeof(BlockType) * CHAR_BIT;
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t block_count = (bit_size + bits_per_block - 1) / bits_per_block;

        auto data = generate_block_data<BlockType>(block_count, 0.9);
        tracking_null_count<> policy;

        for (auto _ : state)
        {
            auto count = policy.count_non_null(data.data(), bit_size, block_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * block_count * sizeof(BlockType)));
    }

    // Benchmark: count_non_null with all zeros
    template <typename BlockType>
    static void BM_CountNonNull_AllZeros(::benchmark::State& state)
    {
        constexpr size_t bits_per_block = sizeof(BlockType) * CHAR_BIT;
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t block_count = (bit_size + bits_per_block - 1) / bits_per_block;

        std::vector<BlockType> data(block_count, 0);
        tracking_null_count<> policy;

        for (auto _ : state)
        {
            auto count = policy.count_non_null(data.data(), bit_size, block_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * block_count * sizeof(BlockType)));
    }

    // Benchmark: count_non_null with all ones
    template <typename BlockType>
    static void BM_CountNonNull_AllOnes(::benchmark::State& state)
    {
        constexpr size_t bits_per_block = sizeof(BlockType) * CHAR_BIT;
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t block_count = (bit_size + bits_per_block - 1) / bits_per_block;

        std::vector<BlockType> data(block_count, static_cast<BlockType>(~BlockType{0}));
        tracking_null_count<> policy;

        for (auto _ : state)
        {
            auto count = policy.count_non_null(data.data(), bit_size, block_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * block_count * sizeof(BlockType)));
    }

    // Benchmark: count_non_null with partial last block
    template <typename BlockType>
    static void BM_CountNonNull_PartialLastBlock(::benchmark::State& state)
    {
        constexpr size_t bits_per_block = sizeof(BlockType) * CHAR_BIT;
        // Use a bit size that doesn't align to block boundary
        const size_t bit_size = static_cast<size_t>(state.range(0)) - (bits_per_block / 2);
        const size_t block_count = (bit_size + bits_per_block - 1) / bits_per_block;

        auto data = generate_block_data<BlockType>(block_count, 0.5);
        tracking_null_count<> policy;

        for (auto _ : state)
        {
            auto count = policy.count_non_null(data.data(), bit_size, block_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * block_count * sizeof(BlockType)));
    }

    // Benchmark: count_non_null with nullptr (edge case)
    template <typename BlockType>
    static void BM_CountNonNull_Nullptr(::benchmark::State& state)
    {
        const size_t bit_size = static_cast<size_t>(state.range(0));
        tracking_null_count<> policy;

        for (auto _ : state)
        {
            auto count = policy.count_non_null(static_cast<const BlockType*>(nullptr), bit_size, 0);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    // Benchmark: initialize_null_count (which uses count_non_null internally)
    template <typename BlockType>
    static void BM_InitializeNullCount(::benchmark::State& state)
    {
        constexpr size_t bits_per_block = sizeof(BlockType) * CHAR_BIT;
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t block_count = (bit_size + bits_per_block - 1) / bits_per_block;

        auto data = generate_block_data<BlockType>(block_count, 0.5);
        tracking_null_count<> policy;

        for (auto _ : state)
        {
            policy.initialize_null_count(data.data(), bit_size, block_count);
            ::benchmark::DoNotOptimize(policy.null_count());
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * block_count * sizeof(BlockType)));
    }

    // Benchmark: recompute_null_count
    template <typename BlockType>
    static void BM_RecomputeNullCount(::benchmark::State& state)
    {
        constexpr size_t bits_per_block = sizeof(BlockType) * CHAR_BIT;
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t block_count = (bit_size + bits_per_block - 1) / bits_per_block;

        auto data = generate_block_data<BlockType>(block_count, 0.5);
        tracking_null_count<> policy;
        policy.initialize_null_count(data.data(), bit_size, block_count);

        for (auto _ : state)
        {
            policy.recompute_null_count(data.data(), bit_size, block_count);
            ::benchmark::DoNotOptimize(policy.null_count());
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * block_count * sizeof(BlockType)));
    }

    constexpr size_t range_min = 1000;
    constexpr size_t range_max = 100000;
    constexpr size_t range_multiplier = 10;

// Macro to register count_non_null benchmarks for a specific block type
#define REGISTER_COUNT_NON_NULL_BENCHMARKS(TYPE)                       \
    BENCHMARK_TEMPLATE(BM_CountNonNull_50Percent, TYPE)                \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_CountNonNull_10Percent, TYPE)                \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_CountNonNull_90Percent, TYPE)                \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_CountNonNull_AllZeros, TYPE)                 \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_CountNonNull_AllOnes, TYPE)                  \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_CountNonNull_PartialLastBlock, TYPE)         \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_CountNonNull_Nullptr, TYPE)                  \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kNanosecond);                              \
    BENCHMARK_TEMPLATE(BM_InitializeNullCount, TYPE)                   \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_RecomputeNullCount, TYPE)                    \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);

    // Register benchmarks for different block types
    REGISTER_COUNT_NON_NULL_BENCHMARKS(std::uint8_t)
    REGISTER_COUNT_NON_NULL_BENCHMARKS(std::uint64_t)

#undef REGISTER_COUNT_NON_NULL_BENCHMARKS

}  // namespace sparrow::benchmark
