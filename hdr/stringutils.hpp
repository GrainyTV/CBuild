#pragma once
#include <string>

namespace String
{
    inline auto rstrip(std::string&& str) -> const std::string&
    {
        if (not str.empty())
        {
            ssize_t i = str.length() - 1;

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
}
