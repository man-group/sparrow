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

#include <memory>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include <benchmark/benchmark.h>

#include "sparrow/primitive_array.hpp"
#include "sparrow/utils/nullable.hpp"


// Arrow headers
#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/builder.h>
#include <arrow/type.h>

namespace
{
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
    std::vector<sparrow::nullable<T>>
    generate_nullable_data(const std::vector<T>& data, double null_probability, std::mt19937& gen)
    {
        std::vector<sparrow::nullable<T>> nullable_data;
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

    template <typename T>
    static void BM_Sparrow_CreateArray(benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);

        for (auto _ : state)
        {
            sparrow::primitive_array<T> array(data);
            benchmark::DoNotOptimize(array);
            benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    template <typename T>
    static void BM_Sparrow_ReadArrayElementAccess(benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        const sparrow::primitive_array<T> array(data);

        T sum = T{};
        size_t index = 0;

        for (auto _ : state)
        {
            const size_t real_index = index % size;
            const auto& element = array[real_index];
            if (element.has_value())
            {
                sum += element.value();
            }
            index++;
            benchmark::DoNotOptimize(sum);
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    }

    // Sparrow Array Reading Benchmarks - Range-based for loop
    template <typename T>
    static void BM_Sparrow_ReadArrayRangeFor(benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);
        sparrow::primitive_array<T> array(data);

        for (auto _ : state)
        {
            T sum = T{};
            for (const auto& element : array)
            {
                if (element.has_value())
                {
                    sum += element.value();
                }
            }
            benchmark::DoNotOptimize(sum);
            benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Arrow Array Creation Benchmarks
    template <typename T>
    static void BM_Arrow_CreateArray(benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);

        for (auto _ : state)
        {
            // Create Arrow array using the builder pattern
            std::shared_ptr<arrow::Array> array;

            if constexpr (std::is_same_v<T, std::int32_t>)
            {
                arrow::Int32Builder builder;
                builder.AppendValues(data);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, std::int64_t>)
            {
                arrow::Int64Builder builder;
                builder.AppendValues(data);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                arrow::FloatBuilder builder;
                builder.AppendValues(data);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                arrow::DoubleBuilder builder;
                builder.AppendValues(data);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, std::uint32_t>)
            {
                arrow::UInt32Builder builder;
                builder.AppendValues(data);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, std::uint64_t>)
            {
                arrow::UInt64Builder builder;
                builder.AppendValues(data);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                arrow::BooleanBuilder builder;
                builder.AppendValues(data);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }

            benchmark::DoNotOptimize(array);
            benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    template <typename T>
    std::shared_ptr<arrow::Array> create_arrow_array(size_t size)
    {
        auto data = generate_sequential_data<T>(size);
        std::shared_ptr<arrow::Array> array;

        auto build_array = [&]<typename U>()
        {
            U builder;
            builder.AppendValues(data);
            auto maybe_array = builder.Finish();
            array = *maybe_array;
        };

        if constexpr (std::is_same_v<T, std::int32_t>)
        {
            build_array.template operator()<arrow::Int32Builder>();
        }
        else if constexpr (std::is_same_v<T, std::int64_t>)
        {
            build_array.template operator()<arrow::Int64Builder>();
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            build_array.template operator()<arrow::FloatBuilder>();
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            build_array.template operator()<arrow::DoubleBuilder>();
        }
        else if constexpr (std::is_same_v<T, std::uint32_t>)
        {
            build_array.template operator()<arrow::UInt32Builder>();
        }
        else if constexpr (std::is_same_v<T, std::uint64_t>)
        {
            build_array.template operator()<arrow::UInt64Builder>();
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            build_array.template operator()<arrow::BooleanBuilder>();
        }
        return array;
    }

    // Arrow Array Reading Benchmarks - Element Access
    template <typename T>
    static void BM_Arrow_ReadArrayElementAccess(benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        const auto array = create_arrow_array<T>(size);
        const auto bench = [&]<typename U>()
        {
            const auto typed_array = std::static_pointer_cast<U>(array);
            T sum = T{};
            size_t index = 0;
            for (auto _ : state)
            {
                const size_t real_index = index % size;
                if (!typed_array->IsNull(real_index))
                {
                    sum += typed_array->Value(real_index);
                }
                index++;
                benchmark::DoNotOptimize(sum);
            }
            state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
        };

        if constexpr (std::is_same_v<T, std::int32_t>)
        {
            bench.template operator()<arrow::Int32Array>();
        }
        else if constexpr (std::is_same_v<T, std::int64_t>)
        {
            bench.template operator()<arrow::Int64Array>();
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            bench.template operator()<arrow::FloatArray>();
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            bench.template operator()<arrow::DoubleArray>();
        }
        else if constexpr (std::is_same_v<T, std::uint32_t>)
        {
            bench.template operator()<arrow::UInt32Array>();
        }
        else if constexpr (std::is_same_v<T, std::uint64_t>)
        {
            bench.template operator()<arrow::UInt64Array>();
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            bench.template operator()<arrow::BooleanArray>();
        }
    }

    // Arrow Array Reading Benchmarks - Raw values access
    template <typename T>
    static void BM_Arrow_ReadArrayRawValues(benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));

        // Create Arrow array
        const std::shared_ptr<arrow::Array> array = create_arrow_array<T>(size);

        const auto bench = [&]<typename U>()
        {
            auto typed_array = std::static_pointer_cast<U>(array);
            if constexpr (std::is_same_v<T, bool>)
            {
                for (auto _ : state)
                {
                    // For bool arrays, we iterate through each bit
                    for (size_t i = 0; i < size; ++i)
                    {
                        volatile bool val = typed_array->Value(i);
                        benchmark::DoNotOptimize(val);
                    }
                }
            }
            else
            {
                const auto* raw_values = typed_array->raw_values();

                T sum = T{};
                for (auto _ : state)
                {
                    for (size_t i = 0; i < size; ++i)
                    {
                        if constexpr (!std::is_same_v<T, bool>)
                        {
                            sum += raw_values[i];
                        }
                        else if constexpr (std::is_same_v<T, bool>)
                        {
                            volatile bool val = raw_values[i];
                            benchmark::DoNotOptimize(val);
                        }
                    }
                    benchmark::DoNotOptimize(sum);
                }
            }
        };

        if constexpr (std::is_same_v<T, std::int32_t>)
        {
            bench.template operator()<arrow::Int32Array>();
        }
        else if constexpr (std::is_same_v<T, std::int64_t>)
        {
            bench.template operator()<arrow::Int64Array>();
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            bench.template operator()<arrow::FloatArray>();
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            bench.template operator()<arrow::DoubleArray>();
        }
        else if constexpr (std::is_same_v<T, std::uint32_t>)
        {
            bench.template operator()<arrow::UInt32Array>();
        }
        else if constexpr (std::is_same_v<T, std::uint64_t>)
        {
            bench.template operator()<arrow::UInt64Array>();
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            bench.template operator()<arrow::BooleanArray>();
        }
        benchmark::ClobberMemory();
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    // Benchmarks with null values
    template <typename T>
    static void BM_Sparrow_CreateArrayWithNulls(benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        std::mt19937 gen(42);  // Fixed seed for reproducibility
        auto data = generate_sequential_data<T>(size);
        auto nullable_data = generate_nullable_data(data, 0.1, gen);  // 10% null values

        for (auto _ : state)
        {
            sparrow::primitive_array<T> array(nullable_data);
            benchmark::DoNotOptimize(array);
            benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

    template <typename T>
    static void BM_Arrow_CreateArrayWithNulls(benchmark::State& state)
    {
        const size_t size = static_cast<size_t>(state.range(0));
        auto data = generate_sequential_data<T>(size);

        std::mt19937 gen(42);  // Fixed seed for reproducibility - same as Sparrow

        // Generate validity vector (10% nulls)
        std::vector<bool> validity;
        validity.reserve(size);
        std::bernoulli_distribution null_dist(0.1);
        for (size_t i = 0; i < size; ++i)
        {
            validity.push_back(!null_dist(gen));  // true means valid, false means null
        }

        for (auto _ : state)
        {
            std::shared_ptr<arrow::Array> array;

            if constexpr (std::is_same_v<T, std::int32_t>)
            {
                arrow::Int32Builder builder;
                builder.AppendValues(data, validity);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, std::int64_t>)
            {
                arrow::Int64Builder builder;
                builder.AppendValues(data, validity);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                arrow::FloatBuilder builder;
                builder.AppendValues(data, validity);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                arrow::DoubleBuilder builder;
                builder.AppendValues(data, validity);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, std::uint32_t>)
            {
                arrow::UInt32Builder builder;
                builder.AppendValues(data, validity);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, std::uint64_t>)
            {
                arrow::UInt64Builder builder;
                builder.AppendValues(data, validity);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                arrow::BooleanBuilder builder;
                builder.AppendValues(data, validity);
                auto maybe_array = builder.Finish();
                array = *maybe_array;
            }

            benchmark::DoNotOptimize(array);
            benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(static_cast<int64_t>(state.iterations() * size));
    }

// Macro to register all benchmarks for a specific type
#define REGISTER_PRIMITIVE_BENCHMARKS(TYPE, NAME)               \
    BENCHMARK_TEMPLATE(BM_Sparrow_CreateArray, TYPE)            \
        ->Name("Sparrow_CreateArray_" NAME)                     \
        ->RangeMultiplier(10)                                   \
        ->Range(100, 100000)                                    \
        ->Unit(benchmark::kMicrosecond);                        \
                                                                \
    BENCHMARK_TEMPLATE(BM_Arrow_CreateArray, TYPE)              \
        ->Name("Arrow_CreateArray_" NAME)                       \
        ->RangeMultiplier(10)                                   \
        ->Range(100, 100000)                                    \
        ->Unit(benchmark::kMicrosecond);                        \
                                                                \
    BENCHMARK_TEMPLATE(BM_Sparrow_ReadArrayElementAccess, TYPE) \
        ->Name("Sparrow_ReadArray_ElementAccess_" NAME)         \
        ->RangeMultiplier(10)                                   \
        ->Range(100, 100000)                                    \
        ->Unit(benchmark::kNanosecond);                         \
                                                                \
    BENCHMARK_TEMPLATE(BM_Arrow_ReadArrayElementAccess, TYPE)   \
        ->Name("Arrow_ReadArray_ElementAccess_" NAME)           \
        ->RangeMultiplier(10)                                   \
        ->Range(100, 100000)                                    \
        ->Unit(benchmark::kNanosecond);                         \
                                                                \
    BENCHMARK_TEMPLATE(BM_Sparrow_ReadArrayRangeFor, TYPE)      \
        ->Name("Sparrow_ReadArray_RangeFor_" NAME)              \
        ->RangeMultiplier(10)                                   \
        ->Range(100, 100000)                                    \
        ->Unit(benchmark::kMicrosecond);                        \
                                                                \
    BENCHMARK_TEMPLATE(BM_Arrow_ReadArrayRawValues, TYPE)       \
        ->Name("Arrow_ReadArray_RawValues_" NAME)               \
        ->RangeMultiplier(10)                                   \
        ->Range(100, 100000)                                    \
        ->Unit(benchmark::kMicrosecond);                        \
                                                                \
    BENCHMARK_TEMPLATE(BM_Sparrow_CreateArrayWithNulls, TYPE)   \
        ->Name("Sparrow_CreateArrayWithNulls_" NAME)            \
        ->RangeMultiplier(10)                                   \
        ->Range(100, 100000)                                    \
        ->Unit(benchmark::kMicrosecond);                        \
                                                                \
    BENCHMARK_TEMPLATE(BM_Arrow_CreateArrayWithNulls, TYPE)     \
        ->Name("Arrow_CreateArrayWithNulls_" NAME)              \
        ->RangeMultiplier(10)                                   \
        ->Range(100, 100000)                                    \
        ->Unit(benchmark::kMicrosecond);

}  // namespace

// Register benchmarks for all types
REGISTER_PRIMITIVE_BENCHMARKS(std::int32_t, "Int32")
REGISTER_PRIMITIVE_BENCHMARKS(std::int64_t, "Int64")
REGISTER_PRIMITIVE_BENCHMARKS(std::uint32_t, "UInt32")
REGISTER_PRIMITIVE_BENCHMARKS(std::uint64_t, "UInt64")
REGISTER_PRIMITIVE_BENCHMARKS(float, "Float")
REGISTER_PRIMITIVE_BENCHMARKS(double, "Double")
REGISTER_PRIMITIVE_BENCHMARKS(bool, "Bool")

#undef REGISTER_PRIMITIVE_BENCHMARKS

BENCHMARK_MAIN();
