#define TOML_IMPLEMENTATION
#define TOML_HEADER_ONLY 0
#define TOML_EXCEPTIONS 0

#include <cstdlib>
#include <iostream>
#include <string_view>

#include "fmt/core.h"
#include "fmt/format.h"
#include "fmt/ostream.h"
#include "fmt/ranges.h"
#include "seq/seq.hpp"
#include "enums.hpp"
#include "toml.hpp"

using Enums::Language;
using TomlHelpers::getTomlProperty;

auto formatSemanticVersion(int major, int minor, int patch) -> std::string
{
    return fmt::format("{:d}.{:d}.{:d}", major, minor, patch);
}

template<>
struct fmt::formatter<toml::node_type> : fmt::ostream_formatter {};

constexpr const char* DEFAULT_COMPILER = "g++";

int main()
{
    const std::string tomllibVersion = formatSemanticVersion(TOML_LIB_MAJOR, TOML_LIB_MINOR, TOML_LIB_PATCH);
    const std::string fmtlibVersion  = formatSemanticVersion(FMT_VERSION / 10000, (FMT_VERSION % 10000) / 100, FMT_VERSION % 100);

    fmt::println("using toml++ version: {:s}", tomllibVersion);
    fmt::println("using fmt version: {:s}", fmtlibVersion);

    const toml::parse_result parsed = toml::parse_file("cbuild.toml");

    if (parsed)
    {
        const toml::table& content = parsed;

        const auto languageStr = getTomlProperty<TomlLookupManner::MANDATORY, std::string>(content, "language");

        const Language language =
            (languageStr == "c") ? Language::C :
            (languageStr == "cpp" || languageStr == "c++") ? Language::CPP :
            Language::UNKNOWN;

        const auto primaryModuleName = getTomlProperty<TomlLookupManner::MANDATORY, std::string>(content, "primary");
        const auto primaryModule     = getTomlProperty<TomlLookupManner::MANDATORY, toml::table>(content, primaryModuleName);

        const auto compilerOpt = getTomlProperty<TomlLookupManner::OPTIONAL, std::string>(content, "compiler");
        const auto compiler    = (compilerOpt.has_value()) ? compilerOpt.value() : DEFAULT_COMPILER;

        const auto cflags  = getTomlProperty<TomlLookupManner::MANDATORY, toml::array>(primaryModule, "cflags");
        const auto sources = getTomlProperty<TomlLookupManner::MANDATORY, toml::array>(primaryModule, "sources");
        const auto output  = getTomlProperty<TomlLookupManner::MANDATORY, std::string>(primaryModule, "output");

        const auto cflagsFlattened  = fmt::format("{:s}", fmt::join(cflags | Seq::map([](const toml::node& node) { return node.value_exact<std::string>().value(); }) | Seq::toVector(), " "));
        const auto sourcesFlattened = fmt::format("{:s}", fmt::join(sources | Seq::map([](const toml::node& node) { return node.value_exact<std::string>().value(); }) | Seq::toVector(), " "));

        const std::string cmd = fmt::format("{:s} {:s} {:s} -o {:s}", compiler, cflagsFlattened, sourcesFlattened, output);
        fmt::println("{:s}", cmd);

        const int shellResult = system(cmd.c_str());
        std::cout << "Shell command exit code: " << shellResult << '\n';
    }
    else
    {
        std::cout << "TOML parse failure: " << parsed.error() << "\n";
    }
}
