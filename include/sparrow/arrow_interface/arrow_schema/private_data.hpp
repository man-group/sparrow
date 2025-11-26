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

#include <optional>
#include <string>
#include <type_traits>

#include "sparrow/arrow_interface/private_data_ownership.hpp"
#include "sparrow/config/config.hpp"
#include "sparrow/utils/contracts.hpp"
#include "sparrow/utils/metadata.hpp"
#include "sparrow/utils/mp_utils.hpp"

namespace sparrow
{
    /**
     * Private data for ArrowSchema.
     *
     * This struct holds the private data for ArrowSchema, including format,
     * name and metadata strings, children, and dictionary. It is used in the
     * Sparrow library.
     */
    class SPARROW_API arrow_schema_private_data : public children_ownership,
                                      public dictionary_ownership
    {
    public:

        using FormatType = std::string;
        using NameType = std::optional<std::string>;
        using MetadataType = std::optional<std::string>;

        arrow_schema_private_data() = delete;
        arrow_schema_private_data(const arrow_schema_private_data&) = delete;
        arrow_schema_private_data(arrow_schema_private_data&&) = delete;
        arrow_schema_private_data& operator=(const arrow_schema_private_data&) = delete;
        arrow_schema_private_data& operator=(arrow_schema_private_data&&) = delete;

        ~arrow_schema_private_data() = default;

        template <class F, class N, class M, std::ranges::input_range CHILDREN_OWNERSHIP>
            requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                     && std::constructible_from<arrow_schema_private_data::NameType, N>
                     && std::constructible_from<arrow_schema_private_data::MetadataType, M>
                     && std::is_same_v<std::ranges::range_value_t<CHILDREN_OWNERSHIP>, bool>
        arrow_schema_private_data(
            F format,
            N name,
            M metadata,
            const CHILDREN_OWNERSHIP& children_ownership,
            bool dictionary_ownership
        );

        template <class F, class N, input_metadata_container M = std::vector<metadata_pair>>
            requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                     && std::constructible_from<arrow_schema_private_data::NameType, N>
        arrow_schema_private_data(F format, N name, M metadata, std::size_t children_size = 0);

        [[nodiscard]] const char* format_ptr() const noexcept;
        [[nodiscard]] FormatType& format() noexcept;
        [[nodiscard]] const char* name_ptr() const noexcept;
        [[nodiscard]] NameType& name() noexcept;
        [[nodiscard]] const char* metadata_ptr() const noexcept;
        [[nodiscard]] MetadataType& metadata() noexcept;

    private:

        FormatType m_format;
        NameType m_name;
        MetadataType m_metadata;
    };

    template <class T>
    constexpr std::optional<std::string> to_optional_string(T&& t)
    {
        if constexpr (std::same_as<std::remove_cvref_t<T>, std::string>)
        {
            return std::forward<T>(t);
        }
        else if constexpr (std::same_as<std::nullopt_t, T>)
        {
            return std::nullopt;
        }
        else if constexpr (std::is_pointer_v<T>)
        {
            if (t == nullptr)
            {
                return std::nullopt;
            }
            else
            {
                return std::string(t);
            }
        }
        else if constexpr (std::ranges::range<T>)
        {
            return std::string(t.cbegin(), t.cend());
        }
        else if constexpr (mpl::is_type_instance_of_v<T, std::optional>)
        {
            if (t.has_value())
            {
                return to_optional_string(*t);
            }
            else
            {
                return std::nullopt;
            }
        }
        else
        {
            static_assert(mpl::dependent_false<T, T>::value, "to_optional_string: unsupported type.");
            mpl::unreachable();
        }
    }

    template <class F, class N, class M, std::ranges::input_range CHILDREN_OWNERSHIP>
        requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                     && std::constructible_from<arrow_schema_private_data::NameType, N>
                     && std::constructible_from<arrow_schema_private_data::MetadataType, M>
                     && std::is_same_v<std::ranges::range_value_t<CHILDREN_OWNERSHIP>, bool>
    arrow_schema_private_data::arrow_schema_private_data(
        F format,
        N name,
        M metadata,
        const CHILDREN_OWNERSHIP& children_ownership_range,
        bool dictionary_ownership_value
    )
        : children_ownership(children_ownership_range)
        , dictionary_ownership(dictionary_ownership_value)
        , m_format(std::move(format))
        , m_name(to_optional_string(std::forward<N>(name)))
        , m_metadata(to_optional_string(std::forward<M>(metadata)))
    {
        SPARROW_ASSERT_TRUE(!m_format.empty())
    }

    template <class F, class N, input_metadata_container M>
        requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                     && std::constructible_from<arrow_schema_private_data::NameType, N>
    arrow_schema_private_data::arrow_schema_private_data(F format, N name, M metadata, std::size_t children_size)
        : children_ownership(children_size)
        , m_format(std::move(format))
        , m_name(to_optional_string(std::forward<N>(name)))
        , m_metadata(get_metadata_from_key_values(metadata))
    {
        SPARROW_ASSERT_TRUE(!m_format.empty())
    }

    [[nodiscard]] inline const char* arrow_schema_private_data::format_ptr() const noexcept
    {
        return m_format.data();
    }

    inline arrow_schema_private_data::FormatType& arrow_schema_private_data::format() noexcept
    {
        return m_format;
    }

    [[nodiscard]] inline const char* arrow_schema_private_data::name_ptr() const noexcept
    {
        if (m_name.has_value())
        {
            return m_name->data();
        }
        return nullptr;
    }

    inline arrow_schema_private_data::NameType& arrow_schema_private_data::name() noexcept
    {
        return m_name;
    }

    [[nodiscard]] inline const char* arrow_schema_private_data::metadata_ptr() const noexcept
    {
        if (m_metadata.has_value())
        {
            return m_metadata->data();
        }
        return nullptr;
    }

    inline arrow_schema_private_data::MetadataType& arrow_schema_private_data::metadata() noexcept
    {
        return m_metadata;
    }

}
