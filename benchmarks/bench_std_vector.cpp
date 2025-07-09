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
#include <optional>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

namespace sparrow::benchmark
{
    // Constants to avoid magic numbers
    constexpr double FLOATING_POINT_MULTIPLIER = 0.1;
    constexpr int RANDOM_SEED = 42;
    constexpr double NULL_PROBABILITY = 0.1;
    constexpr size_t INITIAL_ARRAY_SIZE = 1000;
    constexpr int INSERT_VALUE = 42;

    // Benchmark data generators
    template <typename T>
    std::vector<T> generate_sequential_data(size_t size)
    {
        std::vector<T> data;
        data.reserve(size);

        if constexpr (std::is_same_v<T, bool>)
        {
            for (size_t i = 0; i < size; ++i)
            {
                data.push_back(i % 2 == 0);
            }
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            for (size_t i = 0; i < size; ++i)
            {
                data.push_back(static_cast<T>(i) * static_cast<T>(FLOATING_POINT_MULTIPLIER));
            }
        }
        else
        {
            for (size_t i = 0; i < size; ++i)
            {
                data.push_back(static_cast<T>(i));
            }
        }

        return data;
    }

    template <typename T>
    std::vector<std::optional<T>>
    generate_nullable_data(const std::vector<T>& data, double null_probability, std::mt19937& gen)
    {
        std::vector<std::optional<T>> nullable_data;
        nullable_data.reserve(data.size());

        std::bernoulli_distribution null_dist(null_probability);

        for (const auto& value : data)
        {
            if (null_dist(gen))
            {
                nullable_data.emplace_back();  // null value
            }
            else
            {
                nullable_data.emplace_back(value);
            }
        }

        return nullable_data;
    }

    // Benchmark: Construction from vector
    template <typename T>
    static void BM_StdVector_ConstructFromVector(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);

        for (auto _ : state)
        {
            std::vector<T> vec(data);
            ::benchmark::DoNotOptimize(vec);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Construction with nullable data
    template <typename T>
    static void BM_StdVector_ConstructWithNulls(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        std::mt19937 gen(RANDOM_SEED);  // Fixed seed for reproducibility
        auto data = generate_sequential_data<T>(size);
        auto nullable_data = generate_nullable_data(data, NULL_PROBABILITY, gen);  // 10% null values

        for (auto _ : state)
        {
            std::vector<std::optional<T>> vec(nullable_data);
            ::benchmark::DoNotOptimize(vec);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Element access (operator[])
    template <typename T>
    static void BM_StdVector_ElementAccess(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        std::vector<T> vec(data);

        size_t index = 0;
        T sum = T{};

        for (auto _ : state)
        {
            auto element = vec[index % size];
            if constexpr (!std::is_same_v<T, bool>)
            {
                sum += element;
            }
            index++;
            ::benchmark::DoNotOptimize(sum);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    // Benchmark: Iterator traversal
    template <typename T>
    static void BM_StdVector_IteratorTraversal(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        std::vector<T> vec(data);

        for (auto _ : state)
        {
            T sum = T{};
            for (auto it = vec.begin(); it != vec.end(); ++it)
            {
                if constexpr (!std::is_same_v<T, bool>)
                {
                    sum += *it;
                }
            }
            ::benchmark::DoNotOptimize(sum);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Range-based for loop traversal
    template <typename T>
    static void BM_StdVector_RangeBasedFor(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        std::vector<T> vec(data);

        for (auto _ : state)
        {
            T sum = T{};
            for (const auto& element : vec)
            {
                if constexpr (!std::is_same_v<T, bool>)
                {
                    sum += element;
                }
            }
            ::benchmark::DoNotOptimize(sum);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Value iterator traversal (same as iterator for std::vector)
    template <typename T>
    static void BM_StdVector_ValueIterator(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        std::vector<T> vec(data);

        for (auto _ : state)
        {
            T sum = T{};
            for (auto it = vec.begin(); it != vec.end(); ++it)
            {
                if constexpr (!std::is_same_v<T, bool>)
                {
                    sum += *it;
                }
            }
            ::benchmark::DoNotOptimize(sum);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Insert operation (push_back)
    template <typename T>
    static void BM_StdVector_PushBack(::benchmark::State& state)
    {
        const size_t initial_size = INITIAL_ARRAY_SIZE;
        const size_t insert_count = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            auto initial_data = generate_sequential_data<T>(initial_size);
            std::vector<T> vec(initial_data);

            const T value_to_insert = static_cast<T>(INSERT_VALUE);

            const auto start = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < insert_count; ++i)
            {
                vec.push_back(value_to_insert);
            }
            auto end = std::chrono::high_resolution_clock::now();

            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
            state.SetIterationTime(elapsed_seconds.count());

            ::benchmark::DoNotOptimize(vec);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * insert_count));
    }

    // Benchmark: Copy operation
    template <typename T>
    static void BM_StdVector_Copy(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        std::vector<T> original_vec(data);

        for (auto _ : state)
        {
            std::vector<T> copied_vec(original_vec);
            ::benchmark::DoNotOptimize(copied_vec);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

// Macro to register all benchmarks for a specific type
#define REGISTER_VECTOR_BENCHMARKS(TYPE)                       \
    BENCHMARK_TEMPLATE(BM_StdVector_ConstructFromVector, TYPE) \
        ->RangeMultiplier(10)                                  \
        ->Range(100, 100000)                                   \
        ->Unit(::benchmark::kMicrosecond);                     \
    BENCHMARK_TEMPLATE(BM_StdVector_ConstructWithNulls, TYPE)  \
        ->RangeMultiplier(10)                                  \
        ->Range(100, 100000)                                   \
        ->Unit(::benchmark::kMicrosecond);                     \
    BENCHMARK_TEMPLATE(BM_StdVector_ElementAccess, TYPE)       \
        ->RangeMultiplier(10)                                  \
        ->Range(100, 100000)                                   \
        ->Unit(::benchmark::kNanosecond);                      \
    BENCHMARK_TEMPLATE(BM_StdVector_IteratorTraversal, TYPE)   \
        ->RangeMultiplier(10)                                  \
        ->Range(100, 100000)                                   \
        ->Unit(::benchmark::kMicrosecond);                     \
    BENCHMARK_TEMPLATE(BM_StdVector_RangeBasedFor, TYPE)       \
        ->RangeMultiplier(10)                                  \
        ->Range(100, 100000)                                   \
        ->Unit(::benchmark::kMicrosecond);                     \
    BENCHMARK_TEMPLATE(BM_StdVector_ValueIterator, TYPE)       \
        ->RangeMultiplier(10)                                  \
        ->Range(100, 100000)                                   \
        ->Unit(::benchmark::kMicrosecond);                     \
    BENCHMARK_TEMPLATE(BM_StdVector_PushBack, TYPE)            \
        ->RangeMultiplier(10)                                  \
        ->Range(10, 1000)                                      \
        ->Unit(::benchmark::kMicrosecond)                      \
        ->UseManualTime();                                     \
    BENCHMARK_TEMPLATE(BM_StdVector_Copy, TYPE)                \
        ->RangeMultiplier(10)                                  \
        ->Range(100, 100000)                                   \
        ->Unit(::benchmark::kMicrosecond);

    // Register benchmarks for all types
    REGISTER_VECTOR_BENCHMARKS(std::uint8_t)
    REGISTER_VECTOR_BENCHMARKS(std::uint16_t)
    REGISTER_VECTOR_BENCHMARKS(std::uint32_t)
    REGISTER_VECTOR_BENCHMARKS(std::uint64_t)
    REGISTER_VECTOR_BENCHMARKS(float)
    REGISTER_VECTOR_BENCHMARKS(double)
    REGISTER_VECTOR_BENCHMARKS(bool)

#undef REGISTER_VECTOR_BENCHMARKS

}  // namespace sparrow::benchmark
