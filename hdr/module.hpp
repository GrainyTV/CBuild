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
    std::vector<std::string> sources;
    toml::array cflags;
    std::string output;
    ModuleType type;

public:
    BuildModule(const toml::table& module, const BuildContext& context);

    auto build() const -> void;

    auto generateCompileCommandsJson() const -> void;
};
