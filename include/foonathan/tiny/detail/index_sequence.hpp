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
        {};

        template <std::size_t... Indices>
        using index_sequence = integer_sequence<std::size_t, Indices...>;

        // TODO: cross platform
#if defined(__clang__)
        template <std::size_t Size>
        using make_index_sequence = __make_integer_seq<integer_sequence, std::size_t, Size>;
#elif defined(__GNUC__)
        template <std::size_t Size>
        using make_index_sequence = index_sequence<__integer_pack(Size)...>;
#endif
    } // namespace detail
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_DETAIL_INDEX_SEQUENCE_HPP_INCLUDED
