#pragma once
#include <cstdint>

namespace Enums
{
    enum class Language : std::uint8_t
    {
        C,
        CPP,
        UNKNOWN,
    };

    enum class ModuleType : std::uint8_t
    {
        EXECUTABLE,
        DYNAMIC_LIB,
        STATIC_LIB,
        CUSTOM,
    };

    enum class TomlLookupManner : std::uint8_t
    {
        MANDATORY,
        OPTIONAL,
    };
}
