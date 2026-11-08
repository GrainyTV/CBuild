#pragma once
#include "fmt/base.h"
#include "fmt/ostream.h"
#include "fmt/ranges.h"

#include "toml-ext.hpp"

template<>
struct fmt::formatter<toml::node_type> : fmt::ostream_formatter {};

namespace FmtExt
{
    inline auto formatSemanticVersion(int major, int minor, int patch) -> std::string
    {
        return fmt::format("{:d}.{:d}.{:d}", major, minor, patch);
    }
}
