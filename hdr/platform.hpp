#pragma once
#include <string_view>

#if defined(_WIN32)
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_MACOS 0
    #define PLATFORM_LINUX 0

#elif defined(__APPLE__)
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_MACOS 1
    #define PLATFORM_LINUX 0

#elif defined(__linux__)
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_MACOS 0
    #define PLATFORM_LINUX 1

#else
    #error "Unsupported platform in use"

#endif

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
