#pragma once
#include "seq/seq.hpp"

#include <string>
#include <cstdint>

namespace String
{
    inline auto rstrip(std::string&& str) -> const std::string&
    {
        if (not str.empty())
        {
            int64_t i = str.length() - 1;

            while ((str[i] == '\n' || str[i] == '\r') && i >= 0)
            {
                str.erase(i);
                i -= 1;
            }

            return str;
        }

        return str;
    }

    inline auto contains(const std::string& str, const std::string& what) -> bool
    {
        return str.find(what) != std::string::npos;
    }

    inline auto split(const std::string& str, char splitter) -> IEnumerable<std::string>
    {
        std::string accum;

        for (const char& c : str)
        {
            if (c != splitter)
            {
                accum += c;
            }
            else
            {
                co_yield accum;
                accum.clear();
            }
        }

        if (not accum.empty())
        {
            co_yield accum;
        }
    }
}
