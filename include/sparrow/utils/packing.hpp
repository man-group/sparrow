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

// Use standard C++ attributes for alignment
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
// #    define SPARROW_PACKED_STRUCT __pragma(pack(push, 1))
// #    define SPARROW_PACKED_STRUCT_END __pragma(pack(pop))
// #elif defined(__GNUC__) || defined(__clang__)
#    define SPARROW_PACKED_STRUCT __pragma(pack(push, 1))
#    define SPARROW_PACKED_STRUCT_END __pragma(pack(pop))
#else
#    error "Unsupported compiler"
#endif