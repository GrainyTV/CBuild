#pragma once
#include <cstdint>
#include <filesystem>
#include <string>

#include "toml-ext.hpp"

enum class Language : std::uint8_t
{
    C,
    CPP,
};

struct BuildContext
{
    std::string compiler;
    std::string linker;
    std::string archiver;

    std::filesystem::path sourceDir;
    std::filesystem::path buildDir;

    Language language;

    uint8_t compilerInfo = 0;
};

constexpr uint8_t MSVC_IN_USE  = 0b00000001;
constexpr uint8_t MINGW_IN_USE = 0b00000010;

auto buildContextFrom(const toml::table& globalScope, const std::filesystem::path& sourceDir) -> BuildContext;

auto deduceCompilerInfo(BuildContext& context) -> void;
