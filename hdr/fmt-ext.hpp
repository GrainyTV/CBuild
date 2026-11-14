#pragma once
#include <filesystem>

#include "fmt/base.h"
#include "fmt/ostream.h"
#include "fmt/ranges.h"

#include "toml-ext.hpp"
#include "process.hpp"

#define FMT_VERSION_MAJOR FMT_VERSION / 10000
#define FMT_VERSION_MINOR (FMT_VERSION % 10000) / 100
#define FMT_VERSION_PATCH FMT_VERSION % 100

template<>
struct fmt::formatter<toml::node_type> : fmt::ostream_formatter {};

template<>
struct fmt::formatter<std::filesystem::path> : fmt::ostream_formatter {};

template<>
struct fmt::formatter<Process::Result> : fmt::formatter<std::string_view>
{
    auto format(Process::Result result, fmt::format_context& ctx) const
    {
        std::string content;
        content += "Process::Result\n";
        content += fmt::format("├ ExitCode: {:d}\n", result.exitcode);
        content += fmt::format("├ Output: \"{:s}\"\n", result.output);
        content += fmt::format("└ Error: \"{:s}\"\n", result.error);

        return fmt::formatter<string_view>::format(content, ctx);
    }
};

namespace FmtExt
{
    inline auto println(std::string_view message) -> void
    {
        fmt::println("{:s}", message);
    }
}
