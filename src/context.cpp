#include "context.hpp"
#include <optional>
#include <string_view>
#include "terminate.hpp"
#include "platform.hpp"

using TomlExt::getTomlProperty;

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
    return {
        .compiler =
            getTomlProperty<std::string, TomlLookupManner::MAYBE_SPECIFIED>(globalScope, "compiler")
                .value_or(asString(Platform::defaultApp(Tooling::COMPILER))),

        .linker =
            getTomlProperty<std::string, TomlLookupManner::MAYBE_SPECIFIED>(globalScope, "linker")
                .value_or(asString(Platform::defaultApp(Tooling::LINKER))),

        .archiver =
            getTomlProperty<std::string, TomlLookupManner::MAYBE_SPECIFIED>(globalScope, "archiver")
                .value_or(asString(Platform::defaultApp(Tooling::ARCHIVER))),

        .sourceDir = sourceDir,
        .buildDir  = sourceDir / "out",
        .language  = parseLanguage(getTomlProperty<std::string>(globalScope, "language")),
    };
}
