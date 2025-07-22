#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include <sparrow/array.hpp>
#include <sparrow/json_reader/json_parser.hpp>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    try
    {
        // Test basic sparrow array functionality
        auto arr = sparrow::primitive_array<int>({1, 2, 3, 4, 5});
        std::cout << "Created sparrow array with " << arr.size() << " elements\n";

        // Test JSON parsing with a simple boolean array inspired by primitive.json
        std::string json_str = R"({
            "schema": {
                "fields": [
                    {
                        "name": "bool_test",
                        "type": { "name": "bool" },
                        "nullable": true,
                        "children": []
                    }
                ]
            },
            "batches": [
                {
                    "count": 3,
                    "columns": [
                        {
                            "name": "bool_test",
                            "count": 3,
                            "VALIDITY": [1, 0, 1],
                            "DATA": [true, false, true]
                        }
                    ]
                }
            ]
        })";

        // Parse the JSON string
        nlohmann::json json_data = nlohmann::json::parse(json_str);

        // Build a record batch from the JSON
        auto record_batch = sparrow::json_reader::build_record_batch_from_json(json_data, 1);
        std::cout << "Successfully parsed JSON to record batch with " << record_batch.nb_columns()
                  << " columns\n";

        std::cout << "json_reader test passed!\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error in json_reader test: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
