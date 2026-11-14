#pragma once
#include <string_view>

#include "args/args.hxx"
#include "fmt-ext.hpp"
#include "fmt/compile.h"

namespace CompileTime
{
    namespace _internal
    {
        consteval auto deduceCppVersion() -> std::string_view
        {
            switch (__cplusplus)
            {
                case 202002l:
                    return "C++20";
                case 202302l:
                    return "C++23";
                case 202603l:
                    return "C++26";
                default:
                    return "unknown";
            }
        }

        template<int Major, int Minor, int Patch>
        consteval auto formatSemanticVersion()
        {
            return FMT_STATIC_FORMAT("{:d}.{:d}.{:d}", Major, Minor, Patch);
        }

        constexpr auto TOML_LIB_VERSION_STORAGE = formatSemanticVersion<TOML_LIB_MAJOR, TOML_LIB_MINOR, TOML_LIB_PATCH>();
        constexpr auto FMT_LIB_VERSION_STORAGE  = formatSemanticVersion<FMT_VERSION_MAJOR, FMT_VERSION_MINOR, FMT_VERSION_PATCH>();
        constexpr auto ARGS_LIB_VERSION_STORAGE = formatSemanticVersion<ARGS_VERSION_MAJOR, ARGS_VERSION_MINOR, ARGS_VERSION_PATCH>();
    }

    constexpr std::string_view CPP_STD          = _internal::deduceCppVersion();
    constexpr std::string_view TOML_LIB_VERSION = _internal::TOML_LIB_VERSION_STORAGE.c_str();
    constexpr std::string_view FMT_LIB_VERSION  = _internal::FMT_LIB_VERSION_STORAGE.c_str();
    constexpr std::string_view ARGS_LIB_VERSION = _internal::ARGS_LIB_VERSION_STORAGE.c_str();
}
