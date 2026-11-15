#pragma once
#include <cstdlib>
#include <string_view>
#include "fmt-ext.hpp"
#include "process.hpp"

namespace Terminate
{
    enum class TerminateConditionWhen : std::uint8_t
    {
        AT_CONFIGURE,
        AT_BUILD,
    };

    namespace _internal
    {
        [[noreturn]]
        inline auto exitFailure(std::string_view message, TerminateConditionWhen when = TerminateConditionWhen::AT_CONFIGURE)
        {
            switch (when)
            {
                case TerminateConditionWhen::AT_CONFIGURE:
                    fmt::println(fmt::emphasis::bold, "[CONFIG-FAILURE]");
                    break;
                case TerminateConditionWhen::AT_BUILD:
                    fmt::println(fmt::emphasis::bold, "[BUILD-FAILURE]");
                    break;
            }

            FmtExt::println(message);
            std::exit(1);
        }
    }

    [[noreturn]]
    inline auto usage(std::string_view help)
    {
        fmt::print("{:s}", help);
        std::exit(0);
    }

    [[noreturn]]
    inline auto failedArgs(std::string_view info)
    {
        _internal::exitFailure(fmt::format("Failed to parse command line arguments\n{:s}", info), TerminateConditionWhen::AT_CONFIGURE);
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

    [[noreturn]]
    inline auto withMessage(std::string_view message, TerminateConditionWhen when)
    {
        _internal::exitFailure(message, when);
    }
}
