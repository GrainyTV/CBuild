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
};

auto buildContextFrom(const toml::table& globalScope) -> BuildContext;
