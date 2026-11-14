#pragma once
#include <string_view>

namespace Tooling
{
    constexpr auto platformSpecificDefaultCompiler() -> std::string_view
    {
    #if defined(__linux__)
        return "gcc";
    #elif defined(_WIN32)
        return "cl.exe";
    #endif
    }

    constexpr auto platformSpecificDefaultLinker() -> std::string_view
    {
    #if defined(__linux__)
        return "ld";
    #elif defined(_WIN32)
        return "link.exe";
    #endif
    }

    constexpr auto platformSpecificDefaultArchiver() -> std::string_view
    {
    #if defined(__linux__)
        return "ar";
    #elif defined(_WIN32)
        return "cl.exe";
    #endif
    }
}
