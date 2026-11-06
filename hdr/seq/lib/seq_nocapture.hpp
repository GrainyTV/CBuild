// ┏━━━━━━━━━━━━━━━━━━━┓
// ┃ seq_nocapture.hpp ┃
// ┗━━━━━━━━━━━━━━━━━━━┛
// The purpose of the following functions is to provide one more layer of abstraction to those functions that return
// `IEnumerable<T>` type and require some sort of data parameter. Capture parameters don't really play nice with
// coroutines. Even if they are value parameters, they will be most certainly destroyed by the time a yield happens
// causing them to become dangling. This is explained under CP.51 in the C++ Core Guidelines. To avoid this behavior
// we can introduce these kind of NoCapture "symlink functions" because parameters are copied to the coroutine frame.
#pragma once
#include "ienumerable.hpp"

#include <vector>

namespace Seq::_internal
{
    template<typename T>
    inline auto chunkBySizeNoCapture(IEnumerable<T> sequence, std::size_t size) -> IEnumerable<std::vector<T>>
    {
        std::vector<T> out;
        out.reserve(size);

        for (const auto& elem : sequence)
        {
            out.emplace_back(elem);

            if (out.size() == size)
            {
                co_yield out;
                out.clear();
            }
        }

        if (!out.empty())
        {
            co_yield out;
        }
    }

    template<typename T, typename Predicate>
    inline auto filterNoCapture(IEnumerable<T> sequence, Predicate pred) -> IEnumerable<T>
    {
        for (const auto& elem : sequence)
        {
            if (pred(elem))
            {
                co_yield elem;
            }
        }
    }

    template<bool NeedIndex, typename RetVal, typename T, typename Mapping>
    inline auto mapNoCapture(IEnumerable<T> sequence, Mapping mapping) -> IEnumerable<RetVal>
    {
        std::size_t idx = 0;

        for (const auto& elem : sequence)
        {
            if constexpr (NeedIndex)
            {
                co_yield mapping(elem, idx);
            }
            else
            {
                co_yield mapping(elem);
            }

            ++idx;
        }
    }

    template<typename T>
    inline auto skipNoCapture(IEnumerable<T> sequence, std::size_t count) -> IEnumerable<T>
    {
        std::size_t index = 0;

        for (const auto& elem : sequence)
        {
            if (index >= count)
            {
                co_yield elem;
            }

            ++index;
        }
    }

    template<typename T>
    inline auto takeNoCapture(IEnumerable<T> sequence, std::size_t count) -> IEnumerable<T>
    {
        std::size_t index = 0;

        for (const auto& elem : sequence)
        {
            if (index < count)
            {
                co_yield elem;
            }

            ++index;
        }
    }
}
