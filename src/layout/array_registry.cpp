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

#include "sparrow/layout/array_registry.hpp"

#include <stdexcept>

#include "sparrow/bool8_array.hpp"
#include "sparrow/json_array.hpp"
#include "sparrow/utils/temporal.hpp"
#include "sparrow/uuid_array.hpp"

namespace sparrow
{
    namespace detail
    {
        template <class T>
        cloning_ptr<array_wrapper> make_wrapper_ptr(arrow_proxy proxy)
        {
            return cloning_ptr<array_wrapper>{new array_wrapper_impl<T>(T(std::move(proxy)))};
        }

        template <typename WithTZ, typename WithoutTZ>
        cloning_ptr<array_wrapper> make_timestamp_wrapper(arrow_proxy proxy)
        {
            if (get_timezone(proxy) == nullptr)
            {
                return make_wrapper_ptr<WithoutTZ>(std::move(proxy));
            }
            else
            {
                return make_wrapper_ptr<WithTZ>(std::move(proxy));
            }
        }

        // Recursive helper to register all types from all_data_types array
        template <std::size_t I = 0>
        void register_all_types(array_registry& registry)
        {
            if constexpr (I < all_data_types.size())
            {
                register_type<all_data_types[I]>(registry);
                register_all_types<I + 1>(registry);
            }
        }
    }

    array_registry::array_registry()
    {
        // ===== Register all base types using template metaprogramming =====
        // This iterates over all_data_types array and registers each type automatically
        detail::register_all_types(*this);

        // ===== Register all extension types =====

        // JSON extensions on BINARY, LARGE_BINARY, BINARY_VIEW
        register_extension(
            data_type::BINARY,
            json_extension::EXTENSION_NAME,
            [](arrow_proxy proxy)
            {
                return detail::make_wrapper_ptr<json_array>(std::move(proxy));
            }
        );

        register_extension(
            data_type::LARGE_BINARY,
            json_extension::EXTENSION_NAME,
            [](arrow_proxy proxy)
            {
                return detail::make_wrapper_ptr<big_json_array>(std::move(proxy));
            }
        );

        register_extension(
            data_type::BINARY_VIEW,
            json_extension::EXTENSION_NAME,
            [](arrow_proxy proxy)
            {
                return detail::make_wrapper_ptr<json_view_array>(std::move(proxy));
            }
        );

        // UUID extension on FIXED_WIDTH_BINARY
        register_extension(
            data_type::FIXED_WIDTH_BINARY,
            uuid_extension::EXTENSION_NAME,
            [](arrow_proxy proxy)
            {
                return detail::make_wrapper_ptr<uuid_array>(std::move(proxy));
            }
        );

        // Bool8 extension on UINT8
        register_extension(
            data_type::UINT8,
            bool8_array::EXTENSION_NAME,
            [](arrow_proxy proxy)
            {
                return detail::make_wrapper_ptr<bool8_array>(std::move(proxy));
            }
        );
    }

    array_registry& array_registry::instance()
    {
        static array_registry reg;
        return reg;
    }

    void array_registry::register_base_type(data_type dt, factory_func factory)
    {
        m_base_factories[dt] = std::move(factory);
    }

    void
    array_registry::register_extension(data_type base_type, std::string_view extension_name, factory_func factory)
    {
        register_extension(
            base_type,
            [extension_name](const arrow_proxy& proxy)
            {
                return has_extension_name(proxy, extension_name);
            },
            std::move(factory)
        );
    }

    void
    array_registry::register_extension(data_type base_type, extension_predicate predicate, factory_func factory)
    {
        m_extensions[base_type].emplace_back(std::move(predicate), std::move(factory));
    }

    bool array_registry::extension_entry::matches(const array_wrapper& wrapper) const
    {
        return predicate(wrapper.get_arrow_proxy());
    }

    cloning_ptr<array_wrapper> array_registry::create(arrow_proxy proxy) const
    {
        const auto dt = proxy.data_type();

        // Handle dictionary encoding
        if (proxy.dictionary())
        {
            switch (dt)
            {
                case data_type::INT8:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int8_t>>(std::move(proxy));
                case data_type::UINT8:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint8_t>>(std::move(proxy));
                case data_type::INT16:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int16_t>>(std::move(proxy));
                case data_type::UINT16:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint16_t>>(std::move(proxy));
                case data_type::INT32:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int32_t>>(std::move(proxy));
                case data_type::UINT32:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint32_t>>(std::move(proxy));
                case data_type::INT64:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::int64_t>>(std::move(proxy));
                case data_type::UINT64:
                    return detail::make_wrapper_ptr<dictionary_encoded_array<std::uint64_t>>(std::move(proxy));
                default:
                    throw std::runtime_error("data type of dictionary encoded array must be an integer");
            }
        }

        // Check for extensions first
        auto ext_it = m_extensions.find(dt);
        if (ext_it != m_extensions.end())
        {
            for (const auto& entry : ext_it->second)
            {
                if (entry.predicate(proxy))
                {
                    return entry.factory(std::move(proxy));
                }
            }
        }

        // Fall back to base type
        auto base_it = m_base_factories.find(dt);
        if (base_it != m_base_factories.end())
        {
            return base_it->second(std::move(proxy));
        }

        throw std::runtime_error("Unsupported data type");
    }

    bool array_registry::has_extension_name(const arrow_proxy& proxy, std::string_view extension_name)
    {
        const std::optional<key_value_view> metadata = proxy.metadata();
        if (metadata.has_value())
        {
            const auto it = metadata->find("ARROW:extension:name");
            return it != metadata->end() && (*it).second == extension_name;
        }
        return false;
    }

    // Helper to register a single type using compile-time type information
    template <data_type DT>
    void register_type(array_registry& registry)
    {
        if constexpr (DT == data_type::TIMESTAMP_SECONDS || DT == data_type::TIMESTAMP_MILLISECONDS
                      || DT == data_type::TIMESTAMP_MICROSECONDS || DT == data_type::TIMESTAMP_NANOSECONDS)
        {
            // Special handling for timestamp types with timezone check
            using types = timestamp_type_map<DT>;
            registry.register_base_type(
                DT,
                [](arrow_proxy proxy)
                {
                    return detail::make_timestamp_wrapper<typename types::with_tz, typename types::without_tz>(
                        std::move(proxy)
                    );
                }
            );
        }
        else
        {
            // Standard type registration
            using array_t = array_type_t<DT>;
            registry.register_base_type(
                DT,
                [](arrow_proxy proxy)
                {
                    return detail::make_wrapper_ptr<array_t>(std::move(proxy));
                }
            );
        }
    }
}
