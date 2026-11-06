#pragma once
#pragma GCC diagnostic ignored "-Wsubobject-linkage"

#include "lib/config.hpp"
#include "lib/debug.hpp"
#include "lib/seq_helper.hpp"
#include "lib/seq_nocapture.hpp"
#include "lib/type_inspect_utils.hpp"

#include <optional>
#include <string>

template<typename Func, typename T>
auto operator|(IEnumerable<T>&& enumerable, Func&& function)
{
    return std::forward<Func>(function)(std::move(enumerable));
}

template<typename Func, typename T>
auto operator|(IEnumerable<T>& enumerable, Func&& function)
{
    return std::move(enumerable) | std::forward<Func>(function);
}

template<Seq::_internal::TypeInspect::EnsureIsSeq SeqT, typename Func>
auto operator|(const SeqT& sequence, Func&& function)
{
    return Seq::_internal::wrapAsIEnumerable(sequence) | std::forward<Func>(function);
}

namespace Seq
{
    // `Seq::chunkBySize` divides the elements into chunks of the given size.
    // The last chunk may contain less elements if size was not a factor of length.
    inline auto chunkBySize(std::size_t size)
    {
        return [size]<typename T>(IEnumerable<T> sequence) -> IEnumerable<std::vector<T>>
        {
            return _internal::chunkBySizeNoCapture(std::move(sequence), size);
        };
    }

    // `Seq::contains` tests whether a given element is found in the input sequence.
    template<typename T>
    inline auto contains(const T& needed)
    {
        return [needed](IEnumerable<T> sequence) -> bool
        {
            for (const auto& elem : sequence)
            {
                if (elem == needed)
                {
                    return true;
                }
            }

            return false;
        };
    }

    // `Seq::count` returns how many elements in the input sequence satisfy a predicate.
    // Parameter pred has signature `(T) -> bool`.
    template<typename Predicate>
    inline auto count(Predicate&& pred)
    {
        return [pred = std::forward<Predicate>(pred)]<typename T>(IEnumerable<T> sequence) -> std::size_t
        {
            std::size_t count = 0;

            for (const auto& elem : sequence)
            {
                if (pred(elem))
                {
                    ++count;
                }
            }

            return count;
        };
    }

    // `Seq::exists` is a sibling function of `Seq::forall`.
    // Tests whether AT LEAST one element of the sequence satisfies the predicate.
    // Parameter pred has signature `(T) -> bool`.
    template<typename Predicate>
    inline auto exists(Predicate&& pred)
    {
        return [pred = std::forward<Predicate>(pred)]<typename T>(IEnumerable<T> sequence) -> bool
        {
            for (const auto& elem : sequence)
            {
                if (pred(elem))
                {
                    return true;
                }
            }

            return false;
        };
    }

    // `Seq::filter` returns ALL elements that pass the given predicate.
    // Parameter pred has signature `(T) -> bool`.
    template<typename Predicate>
    inline auto filter(Predicate&& pred)
    {
        return [pred = std::forward<Predicate>(pred)]<typename T>(IEnumerable<T> sequence) -> IEnumerable<T>
        {
            return _internal::filterNoCapture(std::move(sequence), pred);
        };
    }

    template<typename Predicate>
    inline auto findIndex(Predicate&& pred)
    {
        return [pred = std::forward<Predicate>(pred)]<typename T>(IEnumerable<T> sequence) -> std::optional<std::size_t>
        {
            std::size_t index = 0;

            for (const auto& elem : sequence)
            {
                if (pred(elem))
                {
                    return index;
                }

                ++index;
            }

            return {};
        };
    }

    // `Seq::forall` is a sibling function of `Seq::exists`.
    // Tests whether ALL elements of the sequence satisfy the predicate.
    // Parameter pred has signature `(T) -> bool`.
    template<typename Predicate>
    inline auto forall(Predicate&& pred)
    {
        return [pred = std::forward<Predicate>(pred)]<typename T>(IEnumerable<T> sequence) -> bool
        {
            for (const auto& elem : sequence)
            {
                if (!pred(elem))
                {
                    return false;
                }
            }

            return true;
        };
    }

    // `Seq::isEmpty` passes in case a sequence does NOT contain any elements.
    inline auto isEmpty()
    {
        return []<typename T>(IEnumerable<T> sequence) -> bool
        {
            return !(sequence.begin() != sequence.end());
        };
    }

    // `Seq::iter` consumes a sequence by applying a "return value"-less function to each element.
    // Parameter action has signature `(T) -> void`.
    template<typename Action>
    inline auto iter(Action&& action)
    {
        return [action = std::forward<Action>(action)]<typename T>(IEnumerable<T> sequence) -> void
        {
            static_assert(_internal::TypeInspect::IS_INVOKABLE<Action, T>);

            for (const auto& elem : sequence)
            {
                action(elem);
            }
        };
    }

    // `Seq::iteri` is equivalent to `Seq::iter` but provides an extra index parameter to use.
    // Parameter action has signature `(T, size_t) -> void`.
    template<typename Action>
    inline auto iteri(Action&& action)
    {
        return [action = std::forward<Action>(action)]<typename T>(IEnumerable<T> sequence) -> void
        {
            static_assert(_internal::TypeInspect::IS_INVOKABLE<Action, T, std::size_t>);

            std::size_t idx = 0;

            for (const auto& elem : sequence)
            {
                action(elem, idx);
                ++idx;
            }
        };
    }

    // `Seq::length` returns the length of the sequence.
    inline auto length()
    {
        return []<typename T>(IEnumerable<T> sequence) -> std::size_t
        {
            std::size_t length = 0;

            for ([[maybe_unused]] const auto& elem : sequence)
            {
                ++length;
            }

            return length;
        };
    }

    // `Seq::map` applies a transformation to its elements.
    // Parameter mapping has signature `(T) -> U`.
    template<typename Mapping>
    inline auto map(Mapping&& mapping)
    {
        return [mapping = std::forward<Mapping>(mapping)]<typename T>(IEnumerable<T> sequence) -> auto
        {
            static_assert(_internal::TypeInspect::IS_INVOKABLE<Mapping, T>);
            using U = _internal::TypeInspect::ReturnValueOf<Mapping, T>;

            return _internal::mapNoCapture<false, U>(std::move(sequence), mapping);
        };
    }

    // `Seq::mapi` is equivalent to `Seq::map` but provides an extra index parameter to use.
    // Parameter mapping has signature `(T, size_t) -> U`.
    template<typename Mapping>
    inline auto mapi(Mapping&& mapping)
    {
        return [mapping = std::forward<Mapping>(mapping)]<typename T>(IEnumerable<T> sequence) -> auto
        {
            static_assert(_internal::TypeInspect::IS_INVOKABLE<Mapping, T, std::size_t>);
            using U = _internal::TypeInspect::ReturnValueOf<Mapping, T, std::size_t>;

            return _internal::mapNoCapture<true, U>(std::move(sequence), mapping);
        };
    }

    // `Seq::pairwise` returns a sequence where all consecutive elements become paired.
    // e.g. `[1, 2, 3]` would become `[(1, 2), (2, 3)]`.
    inline auto pairwise()
    {
        return []<typename T>(IEnumerable<T> sequence) -> IEnumerable<std::pair<T, T>>
        {
            std::optional<T> previousElem;

            for (const auto& elem : sequence)
            {
                if (previousElem.has_value())
                {
                    co_yield std::make_pair(*previousElem, elem);
                }

                previousElem = elem;
            }
        };
    }

    // `Seq::pairwiseWrap` is equivalent to `Seq::pairwise` but creates an extra pair from the last and first elements.
    // e.g. `[1, 2, 3]` would become `[(1, 2), (2, 3), (3, 1)]`.
    inline auto pairwiseWrap()
    {
        return []<typename T>(IEnumerable<T> sequence) -> IEnumerable<std::pair<T, T>>
        {
            bool atLeastOnePair = false;
            std::optional<T> firstElem;
            std::optional<T> previousElem;

            for (const auto& elem : sequence)
            {
                if (previousElem.has_value())
                {
                    atLeastOnePair = true;
                    co_yield std::make_pair(*previousElem, elem);
                }
                else
                {
                    firstElem = elem;
                }

                previousElem = elem;
            }

            if (atLeastOnePair)
            {
                co_yield std::make_pair(*previousElem, *firstElem);
            }
        };
    }

    // `Seq::range` returns every nth(=step) value from the interval [min, max).
    // Parameter step is allowed to be both positive and negative but NOT zero.
    // This works the same way as Python's built-in range function.
    // Check their docs for more info: https://docs.python.org/3/library/stdtypes.html#range.
    template<_internal::TypeInspect::EnsureIsIntegral T>
    inline auto range(T inclusiveMin, T exclusiveMax, T step) -> IEnumerable<T>
    {
        ASSERT(step != static_cast<T>(0), "Parameter step of `Seq::range` MUST NOT be 0");

        if (step > 0)
        {
            return _internal::rangeIncreasing(inclusiveMin, exclusiveMax, step);
        }

        return _internal::rangeDecreasing(inclusiveMin, exclusiveMax, step);
    }

    // `Seq::range` returns all values from the interval [min, max).
    // This works the same way as Python's built-in range function.
    // Check their docs for more info: https://docs.python.org/3/library/stdtypes.html#range.
    template<_internal::TypeInspect::EnsureIsIntegral T>
    inline auto range(T inclusiveMin, T exclusiveMax) -> IEnumerable<T>
    {
        return Seq::range(inclusiveMin, exclusiveMax, static_cast<T>(1));
    }

    // `Seq::range` returns all values from the interval [0, max).
    // This works the same way as Python's built-in range function.
    // Check their docs for more info: https://docs.python.org/3/library/stdtypes.html#range.
    template<_internal::TypeInspect::EnsureIsIntegral T>
    inline auto range(T exclusiveMax) -> IEnumerable<T>
    {
        return Seq::range(static_cast<T>(0), exclusiveMax);
    }

    // `Seq::reduce` continuously applies a reduction function to the next element of the sequence and
    // an arbitrary initial value.
    // Parameter accum can have pretty much any type. Make sure it matches the return type of reduce.
    // Parameter reduce has signature `(T, typeOf[accum]) -> typeOf[accum]`.
    template<typename Accumulator, typename Reduction>
    inline auto reduce(Accumulator&& accum, Reduction&& reduce)
    {
        return [accum  = std::forward<Accumulator>(accum),
                reduce = std::forward<Reduction>(reduce)]<typename T>(IEnumerable<T> sequence) -> Accumulator
        {
            Accumulator out = accum;

            for (const auto& elem : sequence)
            {
                out = reduce(elem, out);
            }

            return out;
        };
    }

    inline auto skip(std::size_t count)
    {
        return [count]<typename T>(IEnumerable<T> sequence) -> IEnumerable<T>
        {
            return _internal::skipNoCapture(std::move(sequence), count);
        };
    }

    inline auto sort()
    {
        return []<typename T>(IEnumerable<T> sequence) -> IEnumerable<T>
        {
            const auto compare = [](const auto& a, const auto& b) -> bool
            {
                return a < b;
            };

            return _internal::sortElementsBy(std::move(sequence), compare);
        };
    }

    template<typename Mapping>
    inline auto sortBy(Mapping&& mapping)
    {
        return [mapping = std::forward<Mapping>(mapping)]<typename T>(IEnumerable<T> sequence) -> IEnumerable<T>
        {
            using U       = _internal::TypeInspect::ReturnValueOf<Mapping, T>;
            using PairedT = std::pair<T, U>;

            IEnumerable<PairedT> mappedSequence =
                sequence | Seq::map([mapping](const auto& elem) { return std::make_pair(elem, mapping(elem)); });

            const auto compare = [](const auto& pair1, const auto& pair2) -> bool
            {
                const U prop1 = std::get<1>(pair1);
                const U prop2 = std::get<1>(pair2);

                return prop1 < prop2;
            };

            return _internal::sortElementsBy<true, PairedT, T>(std::move(mappedSequence), compare);
        };
    }

    template<typename Mapping>
    inline auto sortByDescending(Mapping&& mapping)
    {
        return [mapping = std::forward<Mapping>(mapping)]<typename T>(IEnumerable<T> sequence) -> IEnumerable<T>
        {
            using U       = _internal::TypeInspect::ReturnValueOf<Mapping, T>;
            using PairedT = std::pair<T, U>;

            IEnumerable<PairedT> mappedSequence =
                sequence | Seq::map([mapping](const auto& elem) { return std::make_pair(elem, mapping(elem)); });

            const auto compare = [](const auto& pair1, const auto& pair2) -> bool
            {
                const U prop1 = std::get<1>(pair1);
                const U prop2 = std::get<1>(pair2);

                return prop1 > prop2;
            };

            return _internal::sortElementsBy<true, PairedT, T>(std::move(mappedSequence), compare);
        };
    }

    inline auto sortDescending()
    {
        return []<typename T>(IEnumerable<T> sequence) -> IEnumerable<T>
        {
            const auto compare = [](const auto& a, const auto& b) -> bool
            {
                return a > b;
            };

            return _internal::sortElementsBy(std::move(sequence), compare);
        };
    }

    // `Seq::sum` returns the sum of the sequence. Supports integrals, float and double.
    // By default it will use the T type of the sequence unless T is smaller than 4 bytes.
    // In those case (e.g. int16_t, char or bool) it uses int32_t.
    // If you know that an overflow will occur you can change UserOverride to a larger type.
    template<typename UserOverride = void>
    inline auto sum()
    {
        using _internal::TypeInspect::EnsureIsSummable;
        using _internal::TypeInspect::FallbackSumInitial;

        return []<EnsureIsSummable T>(IEnumerable<T> sequence) -> auto
        {
            if constexpr (EnsureIsSummable<UserOverride>)
            {
                static_assert(sizeof(UserOverride) >= sizeof(T),
                              "UserOverride type in Seq::sum cannot be smaller than the input's T type");

                return _internal::sum(std::move(sequence), UserOverride{});
            }
            else
            {
                return _internal::sum(std::move(sequence), FallbackSumInitial<T>{});
            }
        };
    }

    // `Seq::tail` returns all elements of the sequence EXCEPT the first one.
    inline auto tail()
    {
        return []<typename T>(IEnumerable<T> sequence) -> IEnumerable<T>
        {
            std::size_t index = 0;

            for (const auto& elem : sequence)
            {
                if (index != 0)
                {
                    co_yield elem;
                }

                ++index;
            }
        };
    }

    inline auto take(std::size_t count)
    {
        return [count]<typename T>(IEnumerable<T> sequence) -> IEnumerable<T>
        {
            return _internal::takeNoCapture(std::move(sequence), count);
        };
    }

    // `Seq::toString` consumes a char sequence by returning its string representation.
    // The initially reserved capacity and shrink parameters are configurable.
    template<std::size_t InitialReserve = 16, bool EnableShrink = false>
    inline auto toString()
    {
        return [](IEnumerable<char> sequence) -> std::string
        {
            std::string out;
            out.reserve(InitialReserve);

            for (const auto& elem : sequence)
            {
                out.append(1, elem);
            }

            if constexpr (EnableShrink)
            {
                out.shrink_to_fit();
            }

            return out;
        };
    }

    // `Seq::toVector` consumes a sequence by returning its vector representation.
    // The initially reserved capacity and shrink parameters are configurable.
    // You might want to look at `Seq::toString` if you have a char sequence.
    template<std::size_t InitialReserve = 16, bool EnableShrink = false>
    inline auto toVector()
    {
        return []<typename T>(IEnumerable<T> sequence) -> std::vector<T>
        {
            std::vector<T> out;
            out.reserve(InitialReserve);

            for (const auto& elem : sequence)
            {
                out.emplace_back(elem);
            }

            if constexpr (EnableShrink)
            {
                out.shrink_to_fit();
            }

            return out;
        };
    }
}
