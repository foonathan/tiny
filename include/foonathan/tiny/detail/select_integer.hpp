// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_DETAIL_SELECT_INTEGER_HPP_INCLUDED
#define FOONATHAN_TINY_DETAIL_SELECT_INTEGER_HPP_INCLUDED

#include <climits>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace foonathan
{
namespace tiny
{
    namespace detail
    {
        template <std::size_t Size, typename = void>
        struct select_integer_impl
        {
            static_assert(Size == 0u, "too many bits");
            using type = void;
        };

        constexpr auto max_uint_bits = sizeof(std::uint_least64_t) * CHAR_BIT;

#define FOONATHAN_TINY_DETAIL_SELECT(Min, Max, Type)                                               \
    template <std::size_t Size>                                                                    \
    struct select_integer_impl<Size,                                                               \
                               typename std::enable_if<(Size > (Min) && Size <= (Max))>::type>     \
    {                                                                                              \
        using type = Type;                                                                         \
    };

        FOONATHAN_TINY_DETAIL_SELECT(0u, 8u, std::uint_least8_t)
        FOONATHAN_TINY_DETAIL_SELECT(8u, 16u, std::uint_least16_t)
        FOONATHAN_TINY_DETAIL_SELECT(16u, 32u, std::uint_least32_t)
        FOONATHAN_TINY_DETAIL_SELECT(32u, max_uint_bits, std::uint_least64_t)

#undef FOONATHAN_TINY_DETAIL_SELECT

        template <std::size_t Size>
        using uint_least_n_t = typename select_integer_impl<Size>::type;
    } // namespace detail
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_DETAIL_SELECT_INTEGER_HPP_INCLUDED
