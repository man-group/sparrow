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

#include <chrono>
#include <functional>
#include <map>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include "sparrow/buffer/dynamic_bitset/dynamic_bitset.hpp"

namespace sparrow::benchmark
{
    // Constants for data generation
    constexpr size_t RANDOM_SEED = 42;
    constexpr double DEFAULT_TRUE_PROBABILITY = 0.5;
    constexpr double DEFAULT_ZERO_PROBABILITY = 0.1;
    constexpr double LOW_TRUE_PROBABILITY = 0.3;  // For null count tests
    constexpr size_t CACHE_SIZE_LIMIT = 10000;
    constexpr size_t INSERT_ERASE_COUNT = 10;
    constexpr size_t POSITION_DIVISOR = 20;

    // Optimized benchmark data generators
    std::vector<bool> generate_bool_data(size_t size, double true_probability = DEFAULT_TRUE_PROBABILITY)
    {
        std::vector<bool> data(size);  // Pre-allocate with size, avoiding reserve + push_back

        // Use fast random number generator for better performance
        std::minstd_rand gen(RANDOM_SEED);  // Linear congruential generator - faster than mt19937

        // Convert probability to threshold for more efficient comparison
        const auto threshold = static_cast<std::minstd_rand::result_type>(
            true_probability * std::minstd_rand::max()
        );

        // Generate data in chunks for better performance
        for (size_t i = 0; i < size; ++i)
        {
            data[i] = gen() < threshold;
        }

        return data;
    }

    std::vector<int> generate_int_data(size_t size, double zero_probability = DEFAULT_ZERO_PROBABILITY)
    {
        std::vector<int> data(size);  // Pre-allocate with size

        // Use faster random number generator
        std::minstd_rand gen(RANDOM_SEED);

        // Convert probabilities to thresholds for efficient comparison
        const auto zero_threshold = static_cast<std::minstd_rand::result_type>(
            zero_probability * std::minstd_rand::max()
        );

        // Pre-compute value range for non-zero elements
        constexpr int min_value = 1;
        constexpr int max_value = 100;
        constexpr int value_range = max_value - min_value + 1;

        for (size_t i = 0; i < size; ++i)
        {
            auto rand_val = gen();
            if (rand_val < zero_threshold)
            {
                data[i] = 0;
            }
            else
            {
                // Use modulo for faster integer generation (sufficient for benchmarks)
                data[i] = min_value + static_cast<int>((rand_val % value_range));
            }
        }

        return data;
    }

    // Cache for frequently used data patterns to avoid regeneration
    template <typename T>
    class DataCache
    {
    private:

        struct CacheKey
        {
            size_t size;
            double probability;

            bool operator<(const CacheKey& other) const
            {
                if (size != other.size)
                {
                    return size < other.size;
                }
                return probability < other.probability;
            }
        };

        inline static std::map<CacheKey, std::vector<T>> cache_;

    public:

        static const std::vector<T>&
        get_or_create(size_t size, double probability, const std::function<std::vector<T>(size_t, double)>& generator)
        {
            CacheKey key{size, probability};
            auto it = cache_.find(key);
            if (it == cache_.end())
            {
                // Only cache small to medium datasets to avoid excessive memory usage
                if (size <= CACHE_SIZE_LIMIT)
                {
                    auto result = cache_.emplace(key, generator(size, probability));
                    return result.first->second;
                }
                else
                {
                    // For large datasets, create a static thread_local variable to avoid repeated allocation
                    thread_local static std::vector<T> large_data;
                    large_data = generator(size, probability);
                    return large_data;
                }
            }
            return it->second;
        }
    };

    // Optimized helper functions that use caching for better performance
    const std::vector<bool>& get_bool_data(size_t size, double true_probability = DEFAULT_TRUE_PROBABILITY)
    {
        return DataCache<bool>::get_or_create(size, true_probability, generate_bool_data);
    }

    const std::vector<int>& get_int_data(size_t size, double zero_probability = DEFAULT_ZERO_PROBABILITY)
    {
        return DataCache<int>::get_or_create(size, zero_probability, generate_int_data);
    }

    // Benchmark: Construction with size only (all false)
    template <typename T>
    static void BM_DynamicBitset_ConstructSizeOnly(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            dynamic_bitset<T> bitset(size);
            ::benchmark::DoNotOptimize(bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Construction with size and value
    template <typename T>
    static void BM_DynamicBitset_ConstructSizeValue(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const bool value = true;

        for (auto _ : state)
        {
            dynamic_bitset<T> bitset(size, value);
            ::benchmark::DoNotOptimize(bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Construction from bool vector
    template <typename T>
    static void BM_DynamicBitset_ConstructFromBoolVector(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(size);

        for (auto _ : state)
        {
            dynamic_bitset<T> bitset(data);
            ::benchmark::DoNotOptimize(bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Construction from int vector (non-zero as true)
    template <typename T>
    static void BM_DynamicBitset_ConstructFromIntVector(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_int_data(size);

        for (auto _ : state)
        {
            dynamic_bitset<T> bitset(data);
            ::benchmark::DoNotOptimize(bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Element access via test()
    template <typename T>
    static void BM_DynamicBitset_Test(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(size);
        dynamic_bitset<T> bitset(data);

        size_t index = 0;
        size_t true_count = 0;

        for (auto _ : state)
        {
            bool value = bitset.test(index % size);
            if (value)
            {
                ++true_count;
            }
            index++;
            ::benchmark::DoNotOptimize(true_count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    // Benchmark: Element access via operator[]
    template <typename T>
    static void BM_DynamicBitset_Subscript(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(size);
        dynamic_bitset<T> bitset(data);

        size_t index = 0;
        size_t true_count = 0;

        for (auto _ : state)
        {
            bool value = bitset[index % size];
            if (value)
            {
                ++true_count;
            }
            index++;
            ::benchmark::DoNotOptimize(true_count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    // Benchmark: Setting bits via set()
    template <typename T>
    static void BM_DynamicBitset_Set(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));

        size_t index = 0;

        for (auto _ : state)
        {
            dynamic_bitset<T> bitset(size, false);
            bitset.set(index % size, true);
            index++;
            ::benchmark::DoNotOptimize(bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    // Benchmark: Setting bits via operator[] assignment
    template <typename T>
    static void BM_DynamicBitset_SubscriptAssign(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));

        size_t index = 0;

        for (auto _ : state)
        {
            dynamic_bitset<T> bitset(size, false);
            bitset[index % size] = true;
            index++;
            ::benchmark::DoNotOptimize(bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    // Benchmark: Iterator traversal
    template <typename T>
    static void BM_DynamicBitset_IteratorTraversal(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(size);
        dynamic_bitset<T> bitset(data);

        for (auto _ : state)
        {
            size_t true_count = 0;
            for (auto it = bitset.begin(); it != bitset.end(); ++it)
            {
                if (*it)
                {
                    ++true_count;
                }
            }
            ::benchmark::DoNotOptimize(true_count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Range-based for loop traversal
    template <typename T>
    static void BM_DynamicBitset_RangeBasedFor(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(size);
        dynamic_bitset<T> bitset(data);

        for (auto _ : state)
        {
            size_t true_count = 0;
            for (bool value : bitset)
            {
                if (value)
                {
                    ++true_count;
                }
            }
            ::benchmark::DoNotOptimize(true_count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Push back operations
    template <typename T>
    static void BM_DynamicBitset_PushBack(::benchmark::State& state)
    {
        const size_t insert_count = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(insert_count);

        for (auto _ : state)
        {
            state.PauseTiming();
            dynamic_bitset<T> bitset;
            state.ResumeTiming();

            auto start = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < insert_count; ++i)
            {
                bitset.push_back(data[i]);
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

            state.SetIterationTime(elapsed_seconds.count());

            ::benchmark::DoNotOptimize(bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * insert_count));
    }

    // Benchmark: Insert operations
    template <typename T>
    static void BM_DynamicBitset_Insert(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(size);

        for (auto _ : state)
        {
            state.PauseTiming();
            dynamic_bitset<T> bitset(data);
            state.ResumeTiming();

            auto start = std::chrono::high_resolution_clock::now();

            // Insert at various positions
            for (size_t i = 0; i < INSERT_ERASE_COUNT; ++i)
            {
                auto pos = bitset.cbegin() + (bitset.size() * i) / POSITION_DIVISOR;
                bitset.insert(pos, true);
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

            state.SetIterationTime(elapsed_seconds.count());

            ::benchmark::DoNotOptimize(bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * INSERT_ERASE_COUNT));
    }

    // Benchmark: Erase operations
    template <typename T>
    static void BM_DynamicBitset_Erase(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(size);

        for (auto _ : state)
        {
            state.PauseTiming();
            dynamic_bitset<T> bitset(data);
            state.ResumeTiming();

            auto start = std::chrono::high_resolution_clock::now();

            // Erase from various positions (but not too many to avoid empty bitset)
            for (size_t i = 0; i < std::min(INSERT_ERASE_COUNT, bitset.size() / 2); ++i)
            {
                if (!bitset.empty())
                {
                    auto pos = bitset.cbegin() + bitset.size() / 4;
                    bitset.erase(pos);
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

            state.SetIterationTime(elapsed_seconds.count());

            ::benchmark::DoNotOptimize(bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(
            static_cast<int64_t>(state.iterations() * std::min(INSERT_ERASE_COUNT, size / 2))
        );
    }

    // Benchmark: Resize operations
    template <typename T>
    static void BM_DynamicBitset_Resize(::benchmark::State& state)
    {
        const size_t initial_size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(initial_size);

        for (auto _ : state)
        {
            dynamic_bitset<T> bitset(data);

            // Resize to different sizes
            bitset.resize(initial_size * 2, true);
            bitset.resize(initial_size / 2);
            bitset.resize(initial_size, false);

            ::benchmark::DoNotOptimize(bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * 3));  // 3 resize operations
    }

    // Benchmark: Copy operation
    template <typename T>
    static void BM_DynamicBitset_Copy(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(size);
        dynamic_bitset<T> original_bitset(data);

        for (auto _ : state)
        {
            dynamic_bitset<T> copied_bitset(original_bitset);
            ::benchmark::DoNotOptimize(copied_bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Move operation
    template <typename T>
    static void BM_DynamicBitset_Move(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(size);

        for (auto _ : state)
        {
            state.PauseTiming();
            dynamic_bitset<T> source_bitset(data);
            state.ResumeTiming();

            dynamic_bitset<T> moved_bitset(std::move(source_bitset));
            ::benchmark::DoNotOptimize(moved_bitset);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: null_count() operation
    template <typename T>
    static void BM_DynamicBitset_NullCount(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(size, LOW_TRUE_PROBABILITY);  // 30% true, 70% false
        dynamic_bitset<T> bitset(data);

        for (auto _ : state)
        {
            auto null_count = bitset.null_count();
            ::benchmark::DoNotOptimize(null_count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    // Benchmark: count_non_null() operation (calculated as size - null_count)
    template <typename T>
    static void BM_DynamicBitset_CountNonNull(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto& data = get_bool_data(size, LOW_TRUE_PROBABILITY);  // 30% true, 70% false
        dynamic_bitset<T> bitset(data);

        for (auto _ : state)
        {
            auto non_null_count = bitset.size() - bitset.null_count();
            ::benchmark::DoNotOptimize(non_null_count);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    constexpr size_t range_min = 10000;
    constexpr size_t range_max = 1000000;
    constexpr size_t range_multiplier = 100;

// Macro to register all benchmarks for a specific block type
#define REGISTER_DYNAMIC_BITSET_BENCHMARKS(TYPE)                       \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_ConstructSizeOnly, TYPE)       \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_ConstructSizeValue, TYPE)      \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_ConstructFromBoolVector, TYPE) \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_ConstructFromIntVector, TYPE)  \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_Test, TYPE)                    \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kNanosecond);                              \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_Subscript, TYPE)               \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kNanosecond);                              \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_Set, TYPE)                     \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kNanosecond);                              \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_SubscriptAssign, TYPE)         \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kNanosecond);                              \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_IteratorTraversal, TYPE)       \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_RangeBasedFor, TYPE)           \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_PushBack, TYPE)                \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond)                              \
        ->UseManualTime();                                             \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_Insert, TYPE)                  \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond)                              \
        ->UseManualTime();                                             \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_Erase, TYPE)                   \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond)                              \
        ->UseManualTime();                                             \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_Resize, TYPE)                  \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_Copy, TYPE)                    \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kMicrosecond);                             \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_Move, TYPE)                    \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kNanosecond);                              \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_NullCount, TYPE)               \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kNanosecond);                              \
    BENCHMARK_TEMPLATE(BM_DynamicBitset_CountNonNull, TYPE)            \
        ->RangeMultiplier(range_multiplier)                            \
        ->Range(range_min, range_max)                                  \
        ->Unit(::benchmark::kNanosecond);

    // Register benchmarks for different block types
    REGISTER_DYNAMIC_BITSET_BENCHMARKS(std::uint8_t)
    // REGISTER_DYNAMIC_BITSET_BENCHMARKS(std::uint16_t)
    // REGISTER_DYNAMIC_BITSET_BENCHMARKS(std::uint32_t)
    // REGISTER_DYNAMIC_BITSET_BENCHMARKS(std::uint64_t)

#undef REGISTER_DYNAMIC_BITSET_BENCHMARKS

}  // namespace sparrow::benchmark
