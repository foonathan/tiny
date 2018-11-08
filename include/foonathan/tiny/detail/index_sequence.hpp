// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_DETAIL_INDEX_SEQUENCE_HPP_INCLUDED
#define FOONATHAN_TINY_DETAIL_INDEX_SEQUENCE_HPP_INCLUDED

#include <cstddef>

namespace foonathan
{
namespace tiny
{
    namespace detail
    {
        template <typename T, T... Indices>
        struct integer_sequence
        {
            using type = integer_sequence<T, Indices...>;
        };

        template <std::size_t... Indices>
        using index_sequence = integer_sequence<std::size_t, Indices...>;

#if defined(__clang__)
        template <std::size_t Size>
        using make_index_sequence = __make_integer_seq<integer_sequence, std::size_t, Size>;
#elif defined(__GNUC__) && __GNUC__ >= 8
        template <std::size_t Size>
        using make_index_sequence = index_sequence<__integer_pack(Size)...>;
#else
        // adapted from https://stackoverflow.com/a/32223343
        template <class Sequence1, class Sequence2>
        struct concat_seq;

        template <std::size_t... I1, std::size_t... I2>
        struct concat_seq<index_sequence<I1...>, index_sequence<I2...>>
        : index_sequence<I1..., (sizeof...(I1) + I2)...>
        {};

        template <size_t N>
        struct make_index_sequence : concat_seq<typename make_index_sequence<N / 2>::type,
                                                typename make_index_sequence<N - N / 2>::type>
        {};
        template <>
        struct make_index_sequence<0> : index_sequence<>
        {};
        template <>
        struct make_index_sequence<1> : index_sequence<0>
        {};
#endif
    } // namespace detail
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_DETAIL_INDEX_SEQUENCE_HPP_INCLUDED
