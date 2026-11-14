#define ARGS_NOEXCEPT

#include <cstdlib>
#include <iostream>
#include <string_view>

#include "args/args.hxx"
#include "constants.hpp"
#include "context.hpp"
#include "module.hpp"
#include "process.hpp"
#include "terminate.hpp"
#include "toml-ext.hpp"

using TomlExt::getTomlProperty;

int main(int argc, char *argv[])
{
    args::ArgumentParser parser{"Sane build solution for C/C++"};
    parser.helpParams.proglineOptions = "[OPTIONS]";
    parser.helpParams.flagindent      = 4;
    parser.helpParams.progindent      = 2;
    parser.helpParams.helpindent      = 20;

    args::Flag help{parser, "help", "prints this help message and exits", {'h', "help"}};
    args::Flag verbose{parser, "verbose", "enables additional logging", {'v', "verbose"}};
    args::ValueFlag<std::string> root{parser, "root", "overrides where to look for cbuild.toml project file", {"root"}};

    if (not parser.ParseCLI(argc, argv))
    {
        fmt::println("Failed to parse CLI arguments ({:s})", parser.GetErrorMsg());
        std::abort();
    }

    if (help)
    {
        Terminate::usage(parser.Help());
    }

    if (verbose)
    {
        fmt::println("Application running on {:s} standard", CompileTime::CPP_STD);
        fmt::println("├ using toml++ version: {:s}", CompileTime::TOML_LIB_VERSION);
        fmt::println("├ using fmt version: {:s}", CompileTime::FMT_LIB_VERSION);
        fmt::println("└ using args version: {:s}", CompileTime::ARGS_LIB_VERSION);
    }

    const toml::parse_result parsed = toml::parse_file("cbuild.toml");

    if (parsed)
    {
        const toml::table& content = parsed;
        const BuildContext context = buildContextFrom(content);

        const auto primaryModuleName = getTomlProperty<std::string>(content, "primary");
        const auto primaryModule     = getTomlProperty<toml::table>(content, primaryModuleName);

        const BuildModule module{primaryModule, context};
        module.generateCompileCommandsJson();
        module.build();
    }
    else
    {
        std::cout << "TOML parse failure: " << parsed.error() << "\n";
    }
}
