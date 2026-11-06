#pragma once
#include <cstring>
#include <type_traits>

namespace Seq::_internal::Selectors
{
    // ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
    // ┃ Types with `begin` and `end` member functions ┃
    // ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
    template<typename Seq, typename = std::void_t<decltype(std::declval<Seq>().begin())>>
    inline auto beginSelector(const Seq& sequence)
    {
        return sequence.begin();
    }

    template<typename Seq, typename = std::void_t<decltype(std::declval<Seq>().end())>>
    inline auto endSelector(const Seq& sequence)
    {
        return sequence.end();
    }

    // ┏━━━━━━━━━━┓
    // ┃ C-string ┃
    // ┗━━━━━━━━━━┛
    inline auto beginSelector(const char* cString) { return cString[0]; }

    inline auto endSelector(const char* cString) { return cString[0] + strlen(cString); }
}
