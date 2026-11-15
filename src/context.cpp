#include "context.hpp"
#include <optional>
#include <string_view>
#include "terminate.hpp"
#include "tooling.hpp"

using TomlExt::getTomlProperty;
namespace fs = std::filesystem;

static auto parseCompiler(const std::optional<std::string>& compilerOrNone) -> std::string
{
    if (compilerOrNone.has_value())
    {
        return compilerOrNone.value();
    }

    return std::string{Tooling::platformSpecificDefaultCompiler()};
}

static auto parseLinker(const std::optional<std::string>& linkerOrNone) -> std::string
{
    if (linkerOrNone.has_value())
    {
        return linkerOrNone.value();
    }

    return std::string{Tooling::platformSpecificDefaultLinker()};
}

static auto parseArchiver(const std::optional<std::string>& archiverOrNone) -> std::string
{
    if (archiverOrNone.has_value())
    {
        return archiverOrNone.value();
    }

    return std::string{Tooling::platformSpecificDefaultArchiver()};
}

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

auto buildContextFrom(const toml::table& globalScope, const std::filesystem::path& sourceDir) -> BuildContext
{
    return {
        .compiler = parseCompiler(getTomlProperty<std::string, TomlLookupManner::OPTIONAL>(globalScope, "compiler")),
        .linker   = parseLinker(getTomlProperty<std::string, TomlLookupManner::OPTIONAL>(globalScope, "linker")),
        .archiver = parseArchiver(getTomlProperty<std::string, TomlLookupManner::OPTIONAL>(globalScope, "archiver")),

        .sourceDir = sourceDir,
        .buildDir  = sourceDir / "out",

        .language = parseLanguage(getTomlProperty<std::string>(globalScope, "language")),
    };
}
