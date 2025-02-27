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

namespace sparrow
{
    /*
     * Class for tracking ownership of the dictionary of an `ArrowArray` or
     * an `ArrowSchema` allocated by sparrow.
     */
    class dictionary_ownership
    {
    public:

        constexpr void set_dictionary_ownership(bool ownership);
        [[nodiscard]] constexpr bool has_dictionary_ownership() const;

    protected:

        constexpr explicit dictionary_ownership(bool ownership)
            : m_has_ownership(ownership)
        {
        }

    private:

        bool m_has_ownership = true;
    };

    /******************
     * Implementation *
     ******************/

    constexpr void dictionary_ownership::set_dictionary_ownership(bool ownership)
    {
        m_has_ownership = ownership;
    }

    [[nodiscard]] constexpr bool dictionary_ownership::has_dictionary_ownership() const
    {
        return m_has_ownership;
    }

}  // namespace sparrow
