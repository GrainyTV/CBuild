#include "context.hpp"
#include "terminate.hpp"
#include "platform.hpp"
#include "stringutils.hpp"

#include <string_view>

static auto parseLanguage(std::string_view language) -> Language
{
    if (language == "c")
    {
        return Language::C;
    }
    else if (language == "c++")
    {
        return Language::CPP;
    }

    Terminate::unknownLanguage(language);
}

static auto asString(std::string_view sv) -> std::string
{
    return {sv.data(), sv.size()};
}

auto buildContextFrom(const toml::table& globalScope, const std::filesystem::path& sourceDir) -> BuildContext
{
    BuildContext context{
        .compiler =
            TomlExt::getTomlProperty<std::string, TomlLookupManner::MAYBE_SPECIFIED>(globalScope, "compiler")
                .value_or(asString(Platform::defaultApp(Tooling::COMPILER))),

        .linker =
            TomlExt::getTomlProperty<std::string, TomlLookupManner::MAYBE_SPECIFIED>(globalScope, "linker")
                .value_or(asString(Platform::defaultApp(Tooling::LINKER))),

        .archiver =
            TomlExt::getTomlProperty<std::string, TomlLookupManner::MAYBE_SPECIFIED>(globalScope, "archiver")
                .value_or(asString(Platform::defaultApp(Tooling::ARCHIVER))),

        .sourceDir = sourceDir,
        .buildDir  = sourceDir / "out",
        .language  = parseLanguage(TomlExt::getTomlProperty<std::string>(globalScope, "language")),
    };

    deduceCompilerInfo(context);

    return context;
}

auto deduceCompilerInfo(BuildContext& context) -> void
{
    if (String::contains(context.compiler, "cl.exe"))
    {
        context.compilerInfo |= MSVC_IN_USE;
    }
    else if (String::contains(context.compiler, "mingw"))
    {
        context.compilerInfo |= MINGW_IN_USE;
    }
}
