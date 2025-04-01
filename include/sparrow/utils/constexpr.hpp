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

namespace sparrow
{
// If using gcc versionn < 12, we define the constexpr keyword to be empty.
#if defined(__GNUC__) && __GNUC__ < 12
#    define SPARROW_CONSTEXPR_GCC_11 inline
#else
#    define SPARROW_CONSTEXPR_GCC_11 constexpr
#endif
}
