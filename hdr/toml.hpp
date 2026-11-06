#pragma once
#include <type_traits>

#include "enums.hpp"
#include "fmt/base.h"
#include "seq/seq.hpp"
#include "toml++/toml.hpp"

using Enums::TomlLookupManner;

namespace TomlHelpers
{
    template<typename T>
    constexpr auto tomlTypeNameOf() -> std::string_view
    {
        using Type = std::remove_cv_t<std::remove_reference_t<T>>;

        if constexpr (std::is_same_v<Type, std::string>)
            return "string";
        else if constexpr (std::is_same_v<Type, int64_t>)
            return "integer";
        else if constexpr (std::is_same_v<Type, double>)
            return "floating-point";
        else if constexpr (std::is_same_v<Type, bool>)
            return "boolean";
        else if constexpr (std::is_same_v<Type, toml::date>)
            return "date";
        else if constexpr (std::is_same_v<Type, toml::time>)
            return "time";
        else if constexpr (std::is_same_v<Type, toml::date_time>)
            return "date-time";
        else if constexpr (std::is_same_v<Type, toml::array>)
            return "array";
        else if constexpr (std::is_same_v<Type, toml::table>)
            return "table";
        else
            return "unknown";
    }

    template<typename T>
    auto checkWhetherPropertyIs(const toml::node_view<const toml::node>& node, std::string_view id) -> void
    {
        if (not node.is<T>())
        {
            fmt::println("TOML property `{:s}` is expected to have type {:s}, but found {:s}", id, tomlTypeNameOf<T>(), node.type());
            std::abort();
        }

        if constexpr (std::is_same_v<T, toml::array>)
        {
            const toml::array elements = *node.as_array();

            const bool areAllElementsStrings =
                elements
                | Seq::forall([](const toml::node& node) { return node.is_string(); });

            if (not areAllElementsStrings)
            {
                fmt::println("Not all elements of TOML array `{:s}` are strings", id);
                std::abort();
            }
        }
    }

    template<typename T>
    auto getTomlPropertyAs(const toml::node_view<const toml::node>& node) -> T
    {
        if constexpr (std::is_same_v<T, toml::table>)
        {
            return *node.as_table();
        }
        else if constexpr (std::is_same_v<T, toml::array>)
        {
            return *node.as_array();
        }
        else
        {
            return node.value_exact<T>().value();
        }
    }

    template<TomlLookupManner Lookup, typename T>
    auto getTomlProperty(const toml::table& table, std::string_view id) -> std::conditional_t<Lookup == TomlLookupManner::MANDATORY, T, std::optional<T>>
    {
        const toml::node_view<const toml::node> node = table[id];

        if constexpr (Lookup == TomlLookupManner::MANDATORY)
        {
            if (not node)
            {
                fmt::println("Mandatory TOML property `{:s}` is missing", id);
                std::abort();
            }

            checkWhetherPropertyIs<T>(node, id);
            return getTomlPropertyAs<T>(node);
        }
        else
        {
            if (not node)
            {
                return std::optional<T>{};
            }

            checkWhetherPropertyIs<T>(node, id);
            return std::optional{getTomlPropertyAs<T>(node)};
        }
    }
}
