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
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include "sparrow/primitive_array.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow::benchmark
{
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
                data.push_back(static_cast<T>(i) * static_cast<T>(0.1));
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
    std::vector<nullable<T>>
    generate_nullable_data(const std::vector<T>& data, double null_probability, std::mt19937& gen)
    {
        std::vector<nullable<T>> nullable_data;
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
    static void BM_PrimitiveArray_ConstructFromVector(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);

        for (auto _ : state)
        {
            primitive_array<T> array(data);
            ::benchmark::DoNotOptimize(array);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Construction with nullable data
    template <typename T>
    static void BM_PrimitiveArray_ConstructWithNulls(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        std::mt19937 gen(42);  // Fixed seed for reproducibility
        auto data = generate_sequential_data<T>(size);
        auto nullable_data = generate_nullable_data(data, 0.1, gen);  // 10% null values

        for (auto _ : state)
        {
            primitive_array<T> array(nullable_data);
            ::benchmark::DoNotOptimize(array);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Element access (operator[])
    template <typename T>
    static void BM_PrimitiveArray_ElementAccess(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        primitive_array<T> array(data);

        size_t index = 0;
        T sum = T{};

        for (auto _ : state)
        {
            auto element = array[index % size];
            if (element.has_value())
            {
                if constexpr (!std::is_same_v<T, bool>)
                {
                    sum += element.value();
                }
            }
            index++;
            ::benchmark::DoNotOptimize(sum);
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    // Benchmark: Iterator traversal
    template <typename T>
    static void BM_PrimitiveArray_IteratorTraversal(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        primitive_array<T> array(data);

        for (auto _ : state)
        {
            T sum = T{};
            for (auto it = array.begin(); it != array.end(); ++it)
            {
                if (it->has_value())
                {
                    if constexpr (!std::is_same_v<T, bool>)
                    {
                        sum += it->value();
                    }
                }
            }
            ::benchmark::DoNotOptimize(sum);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Range-based for loop traversal
    template <typename T>
    static void BM_PrimitiveArray_RangeBasedFor(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        primitive_array<T> array(data);

        for (auto _ : state)
        {
            T sum = T{};
            for (const auto& element : array)
            {
                if (element.has_value())
                {
                    if constexpr (!std::is_same_v<T, bool>)
                    {
                        sum += element.value();
                    }
                }
            }
            ::benchmark::DoNotOptimize(sum);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmark: Value iterator traversal
    template <typename T>
    static void BM_PrimitiveArray_ValueIterator(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        primitive_array<T> array(data);

        for (auto _ : state)
        {
            T sum = T{};
            auto values = array.values();
            for (auto it = values.begin(); it != values.end(); ++it)
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
    static void BM_PrimitiveArray_PushBack(::benchmark::State& state)
    {
        const size_t initial_size = 1000;
        const size_t insert_count = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            auto initial_data = generate_sequential_data<T>(initial_size);
            primitive_array<T> array(initial_data);

            const nullable<T> value_to_insert{static_cast<T>(42)};

            const auto start = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < insert_count; ++i)
            {
                array.push_back(value_to_insert);
            }
            auto end = std::chrono::high_resolution_clock::now();

            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
            state.SetIterationTime(elapsed_seconds.count());

            ::benchmark::DoNotOptimize(array);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * insert_count));
    }

    // Benchmark: Copy operation
    template <typename T>
    static void BM_PrimitiveArray_Copy(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        primitive_array<T> original_array(data);

        for (auto _ : state)
        {
            primitive_array<T> copied_array(original_array);
            ::benchmark::DoNotOptimize(copied_array);
            ::benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

// Macro to register all benchmarks for a specific type
#define REGISTER_PRIMITIVE_BENCHMARKS(TYPE)                         \
    BENCHMARK_TEMPLATE(BM_PrimitiveArray_ConstructFromVector, TYPE) \
        ->RangeMultiplier(10)                                       \
        ->Range(100, 100000)                                        \
        ->Unit(::benchmark::kMicrosecond);                          \
    BENCHMARK_TEMPLATE(BM_PrimitiveArray_ConstructWithNulls, TYPE)  \
        ->RangeMultiplier(10)                                       \
        ->Range(100, 100000)                                        \
        ->Unit(::benchmark::kMicrosecond);                          \
    BENCHMARK_TEMPLATE(BM_PrimitiveArray_ElementAccess, TYPE)       \
        ->RangeMultiplier(10)                                       \
        ->Range(100, 100000)                                        \
        ->Unit(::benchmark::kNanosecond);                           \
    BENCHMARK_TEMPLATE(BM_PrimitiveArray_IteratorTraversal, TYPE)   \
        ->RangeMultiplier(10)                                       \
        ->Range(100, 100000)                                        \
        ->Unit(::benchmark::kMicrosecond);                          \
    BENCHMARK_TEMPLATE(BM_PrimitiveArray_RangeBasedFor, TYPE)       \
        ->RangeMultiplier(10)                                       \
        ->Range(100, 100000)                                        \
        ->Unit(::benchmark::kMicrosecond);                          \
    BENCHMARK_TEMPLATE(BM_PrimitiveArray_ValueIterator, TYPE)       \
        ->RangeMultiplier(10)                                       \
        ->Range(100, 100000)                                        \
        ->Unit(::benchmark::kMicrosecond);                          \
    BENCHMARK_TEMPLATE(BM_PrimitiveArray_PushBack, TYPE)            \
        ->RangeMultiplier(10)                                       \
        ->Range(10, 1000)                                           \
        ->Unit(::benchmark::kMicrosecond)                           \
        ->UseManualTime();                                          \
    BENCHMARK_TEMPLATE(BM_PrimitiveArray_Copy, TYPE)                \
        ->RangeMultiplier(10)                                       \
        ->Range(100, 100000)                                        \
        ->Unit(::benchmark::kMicrosecond);

    // Register benchmarks for all types
    REGISTER_PRIMITIVE_BENCHMARKS(std::uint8_t)
    REGISTER_PRIMITIVE_BENCHMARKS(std::uint16_t)
    REGISTER_PRIMITIVE_BENCHMARKS(std::uint32_t)
    REGISTER_PRIMITIVE_BENCHMARKS(std::uint64_t)
    REGISTER_PRIMITIVE_BENCHMARKS(float)
    REGISTER_PRIMITIVE_BENCHMARKS(double)
    REGISTER_PRIMITIVE_BENCHMARKS(bool)

#undef REGISTER_PRIMITIVE_BENCHMARKS

}  // namespace sparrow::benchmark
