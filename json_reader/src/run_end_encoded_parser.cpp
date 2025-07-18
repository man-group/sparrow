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

#include "sparrow/json_reader/run_end_encoded_parser.hpp"

#include "sparrow/json_reader/json_parser.hpp"
#include "sparrow/json_reader/utils.hpp"

namespace sparrow::json_reader
{
    sparrow::array runendencoded_array_from_json(
        const nlohmann::json& array,
        const nlohmann::json& schema,
        const nlohmann::json& root
    )
    {
        utils::check_type(schema, "runendencoded");
        const std::string name = schema.at("name").get<std::string>();
        auto metadata = utils::get_metadata(schema);
        auto children = get_children_arrays(array, schema, root);
        if (children.size() != 2)
        {
            throw std::runtime_error("Run-end encoded array must have exactly two children arrays");
        }
        return sparrow::array{
            sparrow::run_end_encoded_array{std::move(children[0]), std::move(children[1]), name, std::move(metadata)}
        };
    }
}