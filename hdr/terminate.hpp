#pragma once
#include <cstdlib>
#include <string_view>
#include "fmt-ext.hpp"
#include "process.hpp"

namespace Terminate
{
    namespace _internal
    {
        [[noreturn]]
        inline auto exitFailure(std::string_view message)
        {
            FmtExt::println("[FAILURE]");
            FmtExt::println(message);
            std::exit(1);
        }

        [[noreturn]]
        inline auto exit(std::string_view message)
        {
            FmtExt::println(message);
            std::exit(0);
        }
    }

    [[noreturn]]
    inline auto usage(std::string_view help)
    {
        _internal::exit(help);
    }

    [[noreturn]]
    inline auto invalidModuleType(std::string_view type)
    {
        _internal::exitFailure(fmt::format("Specified module type `{:s}` is invalid", type));
    }

    [[noreturn]]
    inline auto missingFile(std::string_view file)
    {
        _internal::exitFailure(fmt::format("File `{:s}` does not exists", file));
    }

    [[noreturn]]
    inline auto unknownLanguage(std::string_view language)
    {
        _internal::exitFailure(fmt::format("Specified language `{:s}` is invalid", language));
    }

    [[noreturn]]
    inline auto failedProcess(const Process::Result& process)
    {
        _internal::exitFailure(fmt::format("{:s}", process.error));
    }
}
