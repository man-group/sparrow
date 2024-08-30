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
#include <vector>

#include "sparrow/arrow_interface/arrow_schema/smart_pointers.hpp"
#include "sparrow/arrow_interface/arrow_array_schema_utils.hpp"

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
        using ChildrenType = std::optional<std::vector<arrow_schema_shared_ptr>>;
        using DictionaryType = arrow_schema_shared_ptr;

        arrow_schema_private_data() = delete;
        arrow_schema_private_data(const arrow_schema_private_data&) = delete;
        arrow_schema_private_data(arrow_schema_private_data&&) = delete;
        arrow_schema_private_data& operator=(const arrow_schema_private_data&) = delete;
        arrow_schema_private_data& operator=(arrow_schema_private_data&&) = delete;

        ~arrow_schema_private_data() = default;


        template <class F, class N, class M, class C, class D>
            requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                     && std::constructible_from<arrow_schema_private_data::NameType, N>
                     && std::constructible_from<arrow_schema_private_data::MetadataType, M>
                     && std::constructible_from<arrow_schema_private_data::ChildrenType, C>
                     && std::constructible_from<arrow_schema_private_data::DictionaryType, D>
        arrow_schema_private_data(F format, N name, M metadata, C children, D dictionary);

        [[nodiscard]] const char* format() const noexcept;
        [[nodiscard]] const char* name() const noexcept;
        [[nodiscard]] const char* metadata() const noexcept;
        [[nodiscard]] const ChildrenType& children() noexcept;
        [[nodiscard]] ArrowSchema** children_pointers() noexcept;
        [[nodiscard]] const DictionaryType& dictionary() const noexcept;
        [[nodiscard]] ArrowSchema* dictionary_pointer() noexcept;

    private:

        FormatType m_format;
        NameType m_name;
        MetadataType m_metadata;
        ChildrenType m_children;
        std::vector<ArrowSchema*> m_children_pointers;
        DictionaryType m_dictionary;
    };

    template <class F, class N, class M, class C, class D>
        requires std::constructible_from<arrow_schema_private_data::FormatType, F>
                     && std::constructible_from<arrow_schema_private_data::NameType, N>
                     && std::constructible_from<arrow_schema_private_data::MetadataType, M>
                     && std::constructible_from<arrow_schema_private_data::ChildrenType, C>
                     && std::constructible_from<arrow_schema_private_data::DictionaryType, D>
    arrow_schema_private_data::arrow_schema_private_data(F format, N name, M metadata, C children, D dictionary)
        : m_format(std::move(format))
        , m_name(std::move(name))
        , m_metadata(std::move(metadata))
        , m_children(std::move(children))
        , m_children_pointers(to_raw_ptr_vec<ArrowSchema>(m_children))
        , m_dictionary(std::move(dictionary))
    {
    }

    [[nodiscard]] inline const char* arrow_schema_private_data::format() const noexcept
    {
        return m_format.data();
    }

    [[nodiscard]] inline const char* arrow_schema_private_data::name() const noexcept
    {
        if (m_name.has_value())
        {
            return m_name->data();
        }
        return nullptr;
    }

    [[nodiscard]] inline const char* arrow_schema_private_data::metadata() const noexcept
    {
        if (m_metadata.has_value())
        {
            return m_metadata->data();
        }
        return nullptr;
    }

    [[nodiscard]] inline const arrow_schema_private_data::ChildrenType&
    arrow_schema_private_data::children() noexcept
    {
        return m_children;
    }

    [[nodiscard]] inline ArrowSchema** arrow_schema_private_data::children_pointers() noexcept
    {
        return m_children_pointers.data();
    }

    [[nodiscard]] inline const arrow_schema_private_data::DictionaryType&
    arrow_schema_private_data::dictionary() const noexcept
    {
        return m_dictionary;
    }

    [[nodiscard]] inline ArrowSchema* arrow_schema_private_data::dictionary_pointer() noexcept
    {
        if (!m_dictionary)
        {
            return nullptr;
        }
        return m_dictionary.get();
    }

}
