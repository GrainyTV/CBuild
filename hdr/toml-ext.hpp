#pragma once
#define TOML_IMPLEMENTATION
#define TOML_HEADER_ONLY 0
#define TOML_EXCEPTIONS 0

#include <type_traits>

#include "fmt/base.h"
#include "seq/seq.hpp"
#include "toml++/toml.hpp"

enum class TomlLookupManner : std::uint8_t
{
    MANDATORY,
    OPTIONAL,
};

namespace TomlExt
{
    template<typename T>
    constexpr bool IS_TOML_VALUE_TYPE =
        std::is_same_v<T, std::string> ||
        std::is_same_v<T, toml::date> ||
        std::is_same_v<T, toml::time> ||
        std::is_same_v<T, toml::date_time> ||
        std::is_same_v<T, int64_t> ||
        std::is_same_v<T, double> ||
        std::is_same_v<T, bool>;

    template<typename T>
    concept TOML_TYPE =
        IS_TOML_VALUE_TYPE<T> ||
        std::is_same_v<T, toml::array> ||
        std::is_same_v<T, toml::table>;

    template<typename T, bool IsTomlValueType>
    struct RequiredTypePrintable
    {
        const toml::node_type value = T{}.type();
    };

    template<typename T>
    struct RequiredTypePrintable<T, true>
    {
        const toml::node_type value = toml::value<T>{}.type();
    };

    template<typename T>
    auto checkWhetherPropertyIs(const toml::node_view<const toml::node>& node, std::string_view id) -> void
    {
        if (not node.is<T>())
        {
            fmt::println(
                "TOML property `{:s}` is expected to have type {:s}, but found {:s}",
                id,
                RequiredTypePrintable<T, IS_TOML_VALUE_TYPE<T>>{}.value,
                node.type()
            );
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

    template<TOML_TYPE T, TomlLookupManner Lookup = TomlLookupManner::MANDATORY>
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
