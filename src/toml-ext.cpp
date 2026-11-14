#define TOML_IMPLEMENTATION
#include "toml-ext.hpp"

auto TomlExt::flattenStringArray(const toml::array &strings) -> std::string
{
    auto divideEntriesBySpaces = [](const toml::node& node, std::string& accum)
    {
        const std::string nodeValue = node.value_exact<std::string>().value();

        if (accum.empty())
        {
            return accum + nodeValue;
        }
        else
        {
            return accum + ' ' + nodeValue;
        }
    };

    return
        strings
        | Seq::reduce(std::string{}, divideEntriesBySpaces);
}
