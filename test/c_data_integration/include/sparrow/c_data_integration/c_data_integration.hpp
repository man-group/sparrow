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

#include <sparrow/c_interface.hpp>

#include "sparrow/c_data_integration/config.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    SPARROW_C_DATA_INTEGRATION_API const char*
    external_CDataIntegration_ExportSchemaFromJson(const char* json_path, ArrowSchema* out);

    SPARROW_C_DATA_INTEGRATION_API const char*
    external_CDataIntegration_ImportSchemaAndCompareToJson(const char* json_path, ArrowSchema* schema);

    SPARROW_C_DATA_INTEGRATION_API const char*
    external_CDataIntegration_ExportBatchFromJson(const char* json_path, int num_batch, ArrowArray* out);

    SPARROW_C_DATA_INTEGRATION_API const char*
    external_CDataIntegration_ImportBatchAndCompareToJson(const char* json_path, int num_batch, ArrowArray* batch);

    SPARROW_C_DATA_INTEGRATION_API int64_t external_BytesAllocated();

#ifdef __cplusplus
}
#endif
