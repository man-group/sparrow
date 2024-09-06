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

namespace sparrow
{
    /**
     * Private data for ArrowSchema.
     *
     * This struct holds the private data for ArrowSchema, including format,
     * name and metadata strings, children, and dictionary. It is used in the
     * Sparrow library.
     */
    class arrow_schema_private_data
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

        template <class F, class N, class M>
            requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                     && std::constructible_from<arrow_schema_private_data::NameType, N>
                     && std::constructible_from<arrow_schema_private_data::MetadataType, M>
        arrow_schema_private_data(F format, N name, M metadata);

        [[nodiscard]] const char* format_ptr() const noexcept;
        FormatType& format() noexcept;
        [[nodiscard]] const char* name_ptr() const noexcept;
        NameType& name() noexcept;
        [[nodiscard]] const char* metadata_ptr() const noexcept;
        MetadataType& metadata() noexcept;

    private:

        FormatType m_format;
        NameType m_name;
        MetadataType m_metadata;
    };

    template <class F, class N, class M>
        requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                     && std::constructible_from<arrow_schema_private_data::NameType, N>
                     && std::constructible_from<arrow_schema_private_data::MetadataType, M>
    arrow_schema_private_data::arrow_schema_private_data(F format, N name, M metadata)
        : m_format(std::move(format))
        , m_name(std::move(name))
        , m_metadata(std::move(metadata))
    {
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
