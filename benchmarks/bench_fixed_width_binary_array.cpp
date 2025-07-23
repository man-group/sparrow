
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
#include <cstddef>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include "sparrow/fixed_width_binary_array.hpp"
#include "sparrow/utils/nullable.hpp"

namespace sparrow::benchmark
{

    // Generate fixed-width binary data
    std::vector<std::array<std::byte, 8>> generate_fixed_width_binary_data(size_t size)
    {
        std::vector<std::array<std::byte, 8>> data;
        data.reserve(size);
        for (size_t i = 0; i < size; ++i)
        {
            std::array<std::byte, 8> arr{};
            for (size_t j = 0; j < 8; ++j)
            {
                arr[j] = static_cast<std::byte>((i + j) % 256);
            }
            data.push_back(arr);
        }
        return data;
    }

    std::vector<nullable<std::array<std::byte, 8>>> generate_nullable_binary_data(
        const std::vector<std::array<std::byte, 8>>& data,
        double null_probability,
        std::mt19937& gen
    )
    {
        std::vector<nullable<std::array<std::byte, 8>>> nullable_data;
        nullable_data.reserve(data.size());
        std::bernoulli_distribution null_dist(null_probability);
        for (const auto& value : data)
        {
            if (null_dist(gen))
            {
                nullable_data.emplace_back();
            }
            else
            {
                nullable_data.emplace_back(value);
            }
        }
        return nullable_data;
    }

    // Construction from vector
    static void BM_FixedWidthBinaryArray_ConstructFromVector(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_fixed_width_binary_data(size);
        for (auto _ : state)
        {
            fixed_width_binary_array array(data);
            ::benchmark::DoNotOptimize(array);
            ::benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Construction with nullable data
    static void BM_FixedWidthBinaryArray_ConstructWithNulls(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        std::mt19937 gen(42);
        auto data = generate_fixed_width_binary_data(size);
        auto nullable_data = generate_nullable_binary_data(data, 0.1, gen);
        for (auto _ : state)
        {
            fixed_width_binary_array array(nullable_data);
            ::benchmark::DoNotOptimize(array);
            ::benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Element access
    static void BM_FixedWidthBinaryArray_ElementAccess(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_fixed_width_binary_data(size);
        fixed_width_binary_array array(data);
        size_t index = 0;
        std::vector<std::byte> vec;
        for (auto _ : state)
        {
            auto element = array[index % size];
            if (element.has_value())
            {
                // Sum the first byte for demonstration
                const auto ref = element.value();
                vec = std::vector<std::byte>{ref.begin(), ref.end()};
            }
            index++;
            ::benchmark::DoNotOptimize(vec);
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    // Iterator traversal
    static void BM_FixedWidthBinaryArray_IteratorTraversal(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_fixed_width_binary_data(size);
        fixed_width_binary_array array(data);
        for (auto _ : state)
        {
            std::vector<std::byte> vec;
            for (auto it = array.begin(); it != array.end(); ++it)
            {
                if (it->has_value())
                {
                    const auto ref = it->value();
                    vec = std::vector<std::byte>{ref.begin(), ref.end()};
                }
            }
            ::benchmark::DoNotOptimize(vec);
            ::benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Range-based for loop traversal
    static void BM_FixedWidthBinaryArray_RangeBasedFor(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_fixed_width_binary_data(size);
        fixed_width_binary_array array(data);
        for (auto _ : state)
        {
            std::vector<std::byte> vec;
            for (const auto& element : array)
            {
                if (element.has_value())
                {
                    const auto ref = element.value();
                    vec = std::vector<std::byte>{ref.begin(), ref.end()};
                }
            }
            ::benchmark::DoNotOptimize(vec);
            ::benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Value iterator traversal
    static void BM_FixedWidthBinaryArray_ValueIterator(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_fixed_width_binary_data(size);
        fixed_width_binary_array array(data);
        for (auto _ : state)
        {
            uint8_t sum{0};
            auto values = array.values();
            for (auto it = values.begin(); it != values.end(); ++it)
            {
                sum = static_cast<uint8_t>((*it)[0]);  // Sum the first byte for demonstration
            }
            ::benchmark::DoNotOptimize(sum);
            ::benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Insert operation (push_back)
    static void BM_FixedWidthBinaryArray_PushBack(::benchmark::State& state)
    {
        const size_t initial_size = 1000;
        const size_t insert_count = static_cast<size_t>(state.range(0));
        for (auto _ : state)
        {
            auto initial_data = generate_fixed_width_binary_data(initial_size);
            fixed_width_binary_array array(initial_data);
            const nullable<std::array<std::byte, 8>> value_to_insert{std::array<std::byte, 8>{
                static_cast<std::byte>(42),
                static_cast<std::byte>(42),
                static_cast<std::byte>(42),
                static_cast<std::byte>(42),
                static_cast<std::byte>(42),
                static_cast<std::byte>(42),
                static_cast<std::byte>(42),
                static_cast<std::byte>(42)
            }};
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

    // Copy operation
    static void BM_FixedWidthBinaryArray_Copy(::benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_fixed_width_binary_data(size);
        fixed_width_binary_array original_array(data);
        for (auto _ : state)
        {
            fixed_width_binary_array copied_array(original_array);
            ::benchmark::DoNotOptimize(copied_array);
            ::benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

#define REGISTER_FIXED_WIDTH_BINARY_BENCHMARKS()            \
    BENCHMARK(BM_FixedWidthBinaryArray_ConstructFromVector) \
        ->RangeMultiplier(10)                               \
        ->Range(100, 100000)                                \
        ->Unit(::benchmark::kMicrosecond);                  \
    BENCHMARK(BM_FixedWidthBinaryArray_ConstructWithNulls)  \
        ->RangeMultiplier(10)                               \
        ->Range(100, 100000)                                \
        ->Unit(::benchmark::kMicrosecond);                  \
    BENCHMARK(BM_FixedWidthBinaryArray_ElementAccess)       \
        ->RangeMultiplier(10)                               \
        ->Range(100, 100000)                                \
        ->Unit(::benchmark::kNanosecond);                   \
    BENCHMARK(BM_FixedWidthBinaryArray_IteratorTraversal)   \
        ->RangeMultiplier(10)                               \
        ->Range(100, 100000)                                \
        ->Unit(::benchmark::kMicrosecond);                  \
    BENCHMARK(BM_FixedWidthBinaryArray_RangeBasedFor)       \
        ->RangeMultiplier(10)                               \
        ->Range(100, 100000)                                \
        ->Unit(::benchmark::kMicrosecond);                  \
    BENCHMARK(BM_FixedWidthBinaryArray_ValueIterator)       \
        ->RangeMultiplier(10)                               \
        ->Range(100, 100000)                                \
        ->Unit(::benchmark::kMicrosecond);                  \
    BENCHMARK(BM_FixedWidthBinaryArray_PushBack)            \
        ->RangeMultiplier(10)                               \
        ->Range(10, 1000)                                   \
        ->Unit(::benchmark::kMicrosecond)                   \
        ->UseManualTime();                                  \
    BENCHMARK(BM_FixedWidthBinaryArray_Copy)                \
        ->RangeMultiplier(10)                               \
        ->Range(100, 100000)                                \
        ->Unit(::benchmark::kMicrosecond);

    REGISTER_FIXED_WIDTH_BINARY_BENCHMARKS()

}  // namespace sparrow::benchmark
