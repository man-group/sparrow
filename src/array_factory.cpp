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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or mplied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>

#include "sparrow/array_factory.hpp"
#include "sparrow/layout/list_layout/list_array.hpp"
#include "sparrow/layout/struct_layout/struct_array.hpp"
#include "sparrow/layout/dictionary_encoded_array.hpp"
#include "sparrow/layout/primitive_array.hpp"
#include "sparrow/layout/null_array.hpp"
#include "sparrow/layout/variable_size_binary_array.hpp"
#include "sparrow/layout/run_end_encoded_layout/run_end_encoded_array.hpp"
namespace sparrow
{
    namespace detail
    {
        template <class T>
        cloning_ptr<array_wrapper> make_wrapper_ptr(arrow_proxy proxy)
        {
            return cloning_ptr<array_wrapper>{new array_wrapper_impl<T>(T(std::move(proxy))) };
        }
    }

    cloning_ptr<array_wrapper> array_factory(arrow_proxy proxy)
    {
        const auto dt = proxy.data_type();

        if (proxy.dictionary())
        {
            switch(dt)
            {
            case data_type::INT8:
                return detail::make_wrapper_ptr<dictionary_encoded_array<std::int8_t>>(std::move(proxy));
            case data_type::UINT8:
                return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint8_t>>(std::move(proxy));
            case data_type::INT16:
                return detail::make_wrapper_ptr<dictionary_encoded_array<std::int16_t>>(std::move(proxy));
            case data_type::UINT16:
                return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint16_t>>(std::move(proxy));
            case data_type::INT32:
                return detail::make_wrapper_ptr<dictionary_encoded_array<std::int32_t>>(std::move(proxy));
            case data_type::UINT32:
                return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint32_t>>(std::move(proxy));
            case data_type::INT64:
                return detail::make_wrapper_ptr<dictionary_encoded_array<std::int64_t>>(std::move(proxy));
            case data_type::UINT64:
                return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint64_t>>(std::move(proxy));
            default:
                throw std::runtime_error("data datype of dictionary encoded array must be an integer");
            }
        }
        else
        {
            switch(dt)
            {
            case data_type::NA:
                return detail::make_wrapper_ptr<null_array>(std::move(proxy));
            case data_type::BOOL:
                return detail::make_wrapper_ptr<primitive_array<bool>>(std::move(proxy));
            case data_type::INT8:
                return detail::make_wrapper_ptr<primitive_array<std::int8_t>>(std::move(proxy));
            case data_type::UINT8:
                return detail::make_wrapper_ptr<primitive_array<std::uint8_t>>(std::move(proxy));
            case data_type::INT16:
                return detail::make_wrapper_ptr<primitive_array<std::int16_t>>(std::move(proxy));
            case data_type::UINT16:
                return detail::make_wrapper_ptr<primitive_array<std::uint16_t>>(std::move(proxy));
            case data_type::INT32:
                return detail::make_wrapper_ptr<primitive_array<std::int32_t>>(std::move(proxy));
            case data_type::UINT32:
                return detail::make_wrapper_ptr<primitive_array<std::uint32_t>>(std::move(proxy));
            case data_type::INT64:
                return detail::make_wrapper_ptr<primitive_array<std::int64_t>>(std::move(proxy));
            case data_type::UINT64:
                return detail::make_wrapper_ptr<primitive_array<std::uint64_t>>(std::move(proxy));
            case data_type::FLOAT:
                return detail::make_wrapper_ptr<primitive_array<float>>(std::move(proxy));
            case data_type::DOUBLE:
                return detail::make_wrapper_ptr<primitive_array<double>>(std::move(proxy));
            case data_type::LIST:
                return detail::make_wrapper_ptr<list_array>(std::move(proxy));
            case data_type::LARGE_LIST:
                return detail::make_wrapper_ptr<big_list_array>(std::move(proxy));
            case data_type::LIST_VIEW:
                return detail::make_wrapper_ptr<list_view_array>(std::move(proxy));
            case data_type::LARGE_LIST_VIEW:
                return detail::make_wrapper_ptr<big_list_view_array>(std::move(proxy));
            case data_type::FIXED_SIZED_LIST:
                return detail::make_wrapper_ptr<fixed_sized_list_array>(std::move(proxy));
            case data_type::STRUCT:
                return detail::make_wrapper_ptr<struct_array>(std::move(proxy));
            case data_type::STRING:
                return detail::make_wrapper_ptr<variable_size_binary_array<std::string, std::string_view>>(std::move(proxy));
            case data_type::RUN_ENCODED:
                return detail::make_wrapper_ptr<run_end_encoded_array>(std::move(proxy));
            case data_type::FIXED_SIZE_BINARY:
            case data_type::TIMESTAMP:
            case data_type::MAP:
            case data_type::DENSE_UNION:
            case data_type::SPARSE_UNION:
            case data_type::DECIMAL:
            case data_type::FIXED_WIDTH_BINARY:
                throw std::runtime_error("not yet supported data type");
            default:
                throw std::runtime_error("not supported data type");
            }
        }
    }
}
