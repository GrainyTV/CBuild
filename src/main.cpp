#define ARGS_NOEXCEPT

#include "args/args.hxx"
#include "constants.hpp"
#include "context.hpp"
#include "module.hpp"
#include "terminate.hpp"
#include "toml-ext.hpp"

using TomlExt::getTomlProperty;
using Terminate::TerminateConditionWhen;
namespace fs = std::filesystem;

int main(int argc, char *argv[])
{
    args::ArgumentParser parser{"Sane build solution for C/C++"};
    parser.helpParams.proglineOptions   = "[OPTIONS]";
    parser.helpParams.progindent        = 0;
    parser.helpParams.descriptionindent = 2;
    parser.helpParams.optionsString     = " OPTIONS:";
    parser.helpParams.flagindent        = 4;
    parser.helpParams.helpindent        = 20;

    args::Flag help{parser, "help", "prints this help message and exits", {'h', "help"}};
    args::Flag verbose{parser, "verbose", "enables additional logging", {'v', "verbose"}};
    args::ValueFlag<std::string> root{parser, "root", "overrides location of cbuild.toml project file", {"root"}};

    if (not parser.ParseCLI(argc, argv))
    {
        Terminate::failedArgs(parser.GetErrorMsg());
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

    fs::path sourceDir = fs::current_path();

    if (root)
    {
        const std::string& rootFlag = root.Get();

        if (not fs::is_directory(rootFlag))
        {
            Terminate::withMessage(
                fmt::format("Your provided --root flag value \"{:s}\" does not point to an existing directory", rootFlag),
                TerminateConditionWhen::AT_CONFIGURE
            );
        }

        sourceDir = fs::canonical(rootFlag);
    }

    fs::path projectFile = sourceDir / "cbuild.toml";

    if (not fs::is_regular_file(projectFile))
    {
        Terminate::withMessage(
            fmt::format("Source directory {:s} does not contain {:s} project file", sourceDir, projectFile.filename().string()),
            TerminateConditionWhen::AT_CONFIGURE
        );
    }

    const toml::parse_result parsed = toml::parse_file(projectFile.string());

    if (parsed)
    {
        const toml::table& content = parsed;
        const BuildContext context = buildContextFrom(content, sourceDir);

        const auto primaryModuleName = getTomlProperty<std::string>(content, "primary");
        const auto primaryModule     = getTomlProperty<toml::table>(content, primaryModuleName);

        const BuildModule module{primaryModuleName, primaryModule, context};
        module.generateCompileCommandsJson();
        module.build();
    }
    else
    {
        const toml::parse_error& error = parsed.error();
        const toml::source_position at = error.source().begin;

        Terminate::withMessage(
            fmt::format("TOML syntax error encountered at (line {:d}, column {:d})\n{:s}", at.line, at.column, error.description()),
            TerminateConditionWhen::AT_CONFIGURE
        );
    }
}
