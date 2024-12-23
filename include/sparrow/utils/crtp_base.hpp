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

#pragma once

namespace sparrow
{
    /**
     * Base class for CRTP base classes.
     *
     * This class provides convenient derived cast
     * methods for CRTP base classes.
     *
     * @tparam D The derived type.
     */
    template <class D>
    class crtp_base
    {
    protected:

        using derived_type = D;

        derived_type& derived_cast();
        const derived_type& derived_cast() const;
    };

    template <class D>
    auto crtp_base<D>::derived_cast() -> derived_type&
    {
        return static_cast<derived_type&>(*this);
    }

    template <class D>
    auto crtp_base<D>::derived_cast() const -> const derived_type&
    {
        return static_cast<const derived_type&>(*this);
    }
}
