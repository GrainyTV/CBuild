#pragma once
#include "ienumerable.hpp"
#include "type_inspect_utils.hpp"

#include <algorithm>
#include <vector>

namespace Seq::_internal
{
    template<typename Seq>
    auto wrapAsIEnumerable(Seq sequence) -> IEnumerable<TypeInspect::ItemOf<Seq>>
    {
        for (const auto& elem : sequence)
        {
            co_yield elem;
        }
    }

    template<typename T, typename Accum>
    auto sum(IEnumerable<T> sequence, Accum accum) -> Accum
    {
        for (const auto& elem : sequence)
        {
            accum += elem;
        }

        return accum;
    }

    template<typename T>
    auto rangeIncreasing(T inclusiveMin, T exclusiveMax, T step) -> IEnumerable<T>
    {
        for (T i = inclusiveMin; i < exclusiveMax; i += step)
        {
            co_yield i;
        }
    }

    template<typename T>
    auto rangeDecreasing(T inclusiveMin, T exclusiveMax, T step) -> IEnumerable<T>
    {
        for (T i = inclusiveMin; i > exclusiveMax; i += step)
        {
            co_yield i;
        }
    }

    template<bool DiscardCompareProperty = false, typename T, typename U = T, typename Compare>
    auto sortElementsBy(IEnumerable<T> sequence, Compare comp) -> IEnumerable<U>
    {
        std::vector<T> buffer{sequence.begin(), sequence.end()};
        std::sort(buffer.begin(), buffer.end(), comp);

        for (const auto& elem : buffer)
        {
            if constexpr (DiscardCompareProperty)
            {
                co_yield std::get<0>(elem);
            }
            else
            {
                co_yield elem;
            }
        }
    }
}
