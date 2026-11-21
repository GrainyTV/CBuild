#pragma once
#include <cstdlib>
#include <string_view>
#include "fmt-ext.hpp"
#include "stringutils.hpp"
#include "platform.hpp"

#if PLATFORM_WINDOWS
    #include <windows.h>

#endif

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

    [[noreturn]]
    inline auto posixError(std::string_view fn)
    {
        _internal::exitFailure(
            fmt::format(
                R"(POSIX system function "{:s}" failed with errno {:d})" "\n"
                R"(Indicated error is "{:s}")",
                fn, errno, strerror(errno)
            )
        );
    }

    [[noreturn]]
    inline auto winapiError(std::string_view fn)
    {
        int error = GetLastError();

        int dwFlags  = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER;
        char* output = nullptr;

        int formatted = FormatMessage(
            dwFlags,
            nullptr,
            error,
            LANG_SYSTEM_DEFAULT,
            reinterpret_cast<char*>(&output), 255,
            nullptr
        );

        if (formatted == 0)
        {
            Terminate::withMessage(
                fmt::format(
                    "Failed to deduce associated message of error code {:d} in Windows handler\n"
                    "FormatMessage exited with code: {:d}", error, GetLastError()
                ),
                TerminateConditionWhen::AT_BUILD
            );
        }

        std::string message = String::rstrip(output);
        LocalFree(output);

        _internal::exitFailure(
            fmt::format(
                R"(Windows system function "{:s}" failed with error code {:d})" "\n"
                R"(Indicated error is "{:s}")",
                fn, error, message
            ),
            TerminateConditionWhen::AT_BUILD
        );
    }
}
