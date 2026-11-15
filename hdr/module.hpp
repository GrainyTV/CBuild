#pragma once
#include "context.hpp"
#include "toml-ext.hpp"
#include <vector>

enum class ModuleType : std::uint8_t
{
    EXECUTABLE,
    DYNAMIC_LIB,
    STATIC_LIB,
    CUSTOM,
};

class BuildModule
{
private:
    BuildContext context;
    std::vector<std::filesystem::path> sources;
    std::vector<std::filesystem::path> headers;
    toml::array cflags;
    std::string output;
    ModuleType type;

public:
    BuildModule(const std::string& moduleName, const toml::table& module, const BuildContext& context);

    auto build() const -> void;

    auto generateCompileCommandsJson() const -> void;
};
