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

    // Generate random byte data with specified bit density
    std::vector<std::uint8_t> generate_byte_data(size_t byte_count, double true_probability)
    {
        std::vector<std::uint8_t> data(byte_count);
        std::minstd_rand gen(RANDOM_SEED);

        constexpr size_t bits_per_byte = 8;
        const auto threshold = static_cast<std::minstd_rand::result_type>(
            true_probability * std::minstd_rand::max()
        );

        for (size_t i = 0; i < byte_count; ++i)
        {
            std::uint8_t byte = 0;
            for (size_t bit = 0; bit < bits_per_byte; ++bit)
            {
                if (gen() < threshold)
                {
                    byte |= static_cast<std::uint8_t>(1u << bit);
                }
            }
            data[i] = byte;
        }

        return data;
    }

    // Benchmark: count_non_null with 50% bit density
    static void BM_CountNonNull_50Percent(::benchmark::State& state)
    {
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t byte_count = (bit_size + 7) / 8;

        auto data = generate_byte_data(byte_count, 0.5);

        for (auto _ : state)
        {
            auto count = count_non_null(data.data(), bit_size, byte_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * byte_count));
    }

    // Benchmark: count_non_null with 10% bit density (sparse)
    static void BM_CountNonNull_10Percent(::benchmark::State& state)
    {
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t byte_count = (bit_size + 7) / 8;

        auto data = generate_byte_data(byte_count, 0.1);

        for (auto _ : state)
        {
            auto count = count_non_null(data.data(), bit_size, byte_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * byte_count));
    }

    // Benchmark: count_non_null with 90% bit density (dense)
    static void BM_CountNonNull_90Percent(::benchmark::State& state)
    {
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t byte_count = (bit_size + 7) / 8;

        auto data = generate_byte_data(byte_count, 0.9);

        for (auto _ : state)
        {
            auto count = count_non_null(data.data(), bit_size, byte_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * byte_count));
    }

    // Benchmark: count_non_null with all zeros
    static void BM_CountNonNull_AllZeros(::benchmark::State& state)
    {
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t byte_count = (bit_size + 7) / 8;

        std::vector<std::uint8_t> data(byte_count, 0);

        for (auto _ : state)
        {
            auto count = count_non_null(data.data(), bit_size, byte_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * byte_count));
    }

    // Benchmark: count_non_null with all ones
    static void BM_CountNonNull_AllOnes(::benchmark::State& state)
    {
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t byte_count = (bit_size + 7) / 8;

        std::vector<std::uint8_t> data(byte_count, 0xFF);

        for (auto _ : state)
        {
            auto count = count_non_null(data.data(), bit_size, byte_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * byte_count));
    }

    // Benchmark: count_non_null with partial last byte
    static void BM_CountNonNull_PartialLastByte(::benchmark::State& state)
    {
        // Use a bit size that doesn't align to byte boundary
        const size_t bit_size = static_cast<size_t>(state.range(0)) - 4;
        const size_t byte_count = (bit_size + 7) / 8;

        auto data = generate_byte_data(byte_count, 0.5);

        for (auto _ : state)
        {
            auto count = count_non_null(data.data(), bit_size, byte_count);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * byte_count));
    }

    // Benchmark: count_non_null with nullptr (edge case)
    static void BM_CountNonNull_Nullptr(::benchmark::State& state)
    {
        const size_t bit_size = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            auto count = count_non_null(nullptr, bit_size, 0);
            ::benchmark::DoNotOptimize(count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    // Benchmark: initialize_null_count (which uses count_non_null internally)
    static void BM_InitializeNullCount(::benchmark::State& state)
    {
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t byte_count = (bit_size + 7) / 8;

        auto data = generate_byte_data(byte_count, 0.5);
        tracking_null_count<> policy;

        for (auto _ : state)
        {
            policy.initialize_null_count(data.data(), bit_size, byte_count);
            ::benchmark::DoNotOptimize(policy.null_count());
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * byte_count));
    }

    // Benchmark: recompute_null_count
    static void BM_RecomputeNullCount(::benchmark::State& state)
    {
        const size_t bit_size = static_cast<size_t>(state.range(0));
        const size_t byte_count = (bit_size + 7) / 8;

        auto data = generate_byte_data(byte_count, 0.5);
        tracking_null_count<> policy;
        policy.initialize_null_count(data.data(), bit_size, byte_count);

        for (auto _ : state)
        {
            policy.recompute_null_count(data.data(), bit_size, byte_count);
            ::benchmark::DoNotOptimize(policy.null_count());
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * bit_size));
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * byte_count));
    }

    constexpr size_t range_min = 1000;
    constexpr size_t range_max = 10000000;
    constexpr size_t range_multiplier = 10;

    BENCHMARK(BM_CountNonNull_50Percent)
        ->RangeMultiplier(range_multiplier)
        ->Range(range_min, range_max)
        ->Unit(::benchmark::kMicrosecond);

    BENCHMARK(BM_CountNonNull_10Percent)
        ->RangeMultiplier(range_multiplier)
        ->Range(range_min, range_max)
        ->Unit(::benchmark::kMicrosecond);

    BENCHMARK(BM_CountNonNull_90Percent)
        ->RangeMultiplier(range_multiplier)
        ->Range(range_min, range_max)
        ->Unit(::benchmark::kMicrosecond);

    BENCHMARK(BM_CountNonNull_AllZeros)
        ->RangeMultiplier(range_multiplier)
        ->Range(range_min, range_max)
        ->Unit(::benchmark::kMicrosecond);

    BENCHMARK(BM_CountNonNull_AllOnes)
        ->RangeMultiplier(range_multiplier)
        ->Range(range_min, range_max)
        ->Unit(::benchmark::kMicrosecond);

    BENCHMARK(BM_CountNonNull_PartialLastByte)
        ->RangeMultiplier(range_multiplier)
        ->Range(range_min, range_max)
        ->Unit(::benchmark::kMicrosecond);

    BENCHMARK(BM_CountNonNull_Nullptr)
        ->RangeMultiplier(range_multiplier)
        ->Range(range_min, range_max)
        ->Unit(::benchmark::kNanosecond);

    BENCHMARK(BM_InitializeNullCount)
        ->RangeMultiplier(range_multiplier)
        ->Range(range_min, range_max)
        ->Unit(::benchmark::kMicrosecond);

    BENCHMARK(BM_RecomputeNullCount)
        ->RangeMultiplier(range_multiplier)
        ->Range(range_min, range_max)
        ->Unit(::benchmark::kMicrosecond);

}  // namespace sparrow::benchmark
