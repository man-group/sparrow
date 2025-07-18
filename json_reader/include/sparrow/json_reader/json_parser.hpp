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

#pragma once

#include <vector>

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
#include <nlohmann/json.hpp>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include <sparrow/array.hpp>
#include <sparrow/record_batch.hpp>

#include "sparrow/json_reader/config.hpp"

namespace sparrow::json_reader
{
    SPARROW_JSON_READER_API sparrow::array build_array_from_json(
        const nlohmann::json& array,
        const nlohmann::json& schema,
        const nlohmann::json& root,
        bool check_dictionary = true
    );

    SPARROW_JSON_READER_API std::vector<sparrow::array>
    get_children_arrays(const nlohmann::json& array, const nlohmann::json& schema, const nlohmann::json& root);

    SPARROW_JSON_READER_API sparrow::array dictionary_encode_array_from_json(
        const nlohmann::json& array,
        const nlohmann::json& schema,
        const nlohmann::json& root
    );

    SPARROW_JSON_READER_API sparrow::record_batch build_record_batch_from_json(const nlohmann::json& root, size_t num_batches);
}

// namespace sparrow::json_reader::json_parser