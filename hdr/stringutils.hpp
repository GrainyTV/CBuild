#pragma once
#include <string>

namespace String
{
    inline auto rstrip(std::string&& str) -> const std::string&
    {
        if (not str.empty())
        {
            int i = str.length() - 1;

            while (str[i] == '\n' && i >= 0)
            {
                str.erase(i);
                i -= 1;
            }

            return str;
        }

        return str;
    }
}
