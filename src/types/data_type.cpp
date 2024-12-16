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

#include "sparrow/types/data_type.hpp"

namespace sparrow
{
    // get the bit width for decimal value type from format
    std::size_t num_bytes_for_decimal(const char* format)
    {
        //    d:19,10     -> 16 bytes / 128 bits
        //    d:38,10,32  -> 4 bytes / 32 bits
        //    d:38,10,64  -> 8 bytes / 64 bits
        //    d:38,10,128 -> 16 bytes / 128 bits
        //    d:38,10,256 -> 32 bytes / 256 bits

        // count the number of commas
        // const auto len = std::strlen(format);
        const auto num_commas = std::count(format, format + std::strlen(format), ',');

        if (num_commas <= 1)
        {
            return 16;
        }
        else
        {
            // get the position of second comma
            const auto second_comma_ptr = std::strrchr(format, ',');
            if (!all_digits(std::string_view(second_comma_ptr + 1)))
            {
                std::stringstream ss;
                ss << "Invalid format for decimal in `" << format << "` not all digits in "
                   << std::string_view(second_comma_ptr + 1);
                throw std::runtime_error(ss.str());
            }
            // get substring after second comma to end
            const auto num_bits = static_cast<std::size_t>(std::atoi(second_comma_ptr + 1));

            if (!(num_bits == 32 || num_bits == 64 || num_bits == 128 || num_bits == 256))
            {
                std::stringstream ss;
                ss << "Invalid number of bits for decimal: " << num_bits << " in `" << format << "`";
                throw std::runtime_error(ss.str());
            }
            return num_bits / 8;
        }
    }


}  // namespace sparrow