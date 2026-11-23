#pragma once
#include <string_view>
#include <cstdint>

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

enum class Tooling : std::uint8_t
{
    COMPILER,
    LINKER,
    ARCHIVER,
};

namespace Platform
{
    consteval auto defaultApp(Tooling tooling) -> std::string_view
    {
        switch (tooling)
        {
            case Tooling::COMPILER:
            #if PLATFORM_WINDOWS
                return "cl.exe";
            #elif PLATFORM_MACOS
                return "clang";
            #else
                return "gcc";
            #endif
                break;

            case Tooling::LINKER:
            #if PLATFORM_WINDOWS
                return "link.exe";
            #elif PLATFORM_MACOS
                return "ld64";
            #else
                return "ld";
            #endif
                break;

            case Tooling::ARCHIVER:
            #if PLATFORM_WINDOWS
                return "lib.exe";
            #elif PLATFORM_MACOS
                return "llvm-ar";
            #else
                return "ar";
            #endif
                break;
        }
    }

    consteval auto objectFileExtension() -> std::string_view
    {
    #if PLATFORM_WINDOWS
        return ".obj";
    #else
        return ".o";
    #endif
    }

    consteval auto multiplePathsSeparator() -> char
    {
    #if PLATFORM_WINDOWS
        return ';';
    #else
        return ':';
    #endif
    }
}
