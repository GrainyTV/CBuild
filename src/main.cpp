#define ARGS_NOEXCEPT

#include <cstdlib>
#include <iostream>
#include <string_view>

#include "args/args.hxx"
#include "seq/seq.hpp"
#include "enums.hpp"
#include "fmt-ext.hpp"
#include "toml-ext.hpp"

using Enums::Language;
using TomlExt::getTomlProperty;

constexpr const char* DEFAULT_COMPILER = "g++";

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

int main(int argc, char *argv[])
{
    args::ArgumentParser parser{"Sane build solution for C/C++"};
    args::Flag help{parser, "help", "prints this help message and exits", {'h', "help"}};
    args::Flag verbose{parser, "verbose", "enables additional logging", {'v', "verbose"}};

    if (not parser.ParseCLI(argc, argv))
    {
        fmt::println("Failed to parse CLI arguments ({:s})", parser.GetErrorMsg());
        std::abort();
    }

    if (help)
    {
        fmt::println("{:s}", parser.Help());
        std::abort();
    }

    if (verbose)
    {
        // CPP Version
        // ---
        static constexpr auto cppVersion = deduceCppVersion();

        fmt::println("Application running on {:s} standard", cppVersion);

        // Library Versions
        // ---
        const std::string tomlLibVersion = FmtExt::formatSemanticVersion(TOML_LIB_MAJOR, TOML_LIB_MINOR, TOML_LIB_PATCH);
        const std::string fmtLibVersion  = FmtExt::formatSemanticVersion(FMT_VERSION / 10000, (FMT_VERSION % 10000) / 100, FMT_VERSION % 100);
        const std::string argsLibVersion = FmtExt::formatSemanticVersion(ARGS_VERSION_MAJOR, ARGS_VERSION_MINOR, ARGS_VERSION_PATCH);

        fmt::println("using toml++ version: {:s}", tomlLibVersion);
        fmt::println("using fmt version: {:s}", fmtLibVersion);
        fmt::println("using args version: {:s}", argsLibVersion);
    }

    const toml::parse_result parsed = toml::parse_file("cbuild.toml");

    if (parsed)
    {
        const toml::table& content = parsed;

        const auto languageStr = getTomlProperty<std::string>(content, "language");

        const Language language =
            (languageStr == "c") ? Language::C :
            (languageStr == "cpp" || languageStr == "c++") ? Language::CPP :
            Language::UNKNOWN;

        const auto primaryModuleName = getTomlProperty<std::string>(content, "primary");
        const auto primaryModule     = getTomlProperty<toml::table>(content, primaryModuleName);

        const auto compilerOpt = getTomlProperty<std::string, TomlLookupManner::OPTIONAL>(content, "compiler");
        const auto compiler    = compilerOpt.has_value() ? compilerOpt.value() : DEFAULT_COMPILER;

        const auto cflags  = getTomlProperty<toml::array>(primaryModule, "cflags");
        const auto sources = getTomlProperty<toml::array>(primaryModule, "sources");
        const auto output  = getTomlProperty<std::string>(primaryModule, "output");

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
