// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_DETAIL_ILOG2_HPP_INCLUDED
#define FOONATHAN_TINY_DETAIL_ILOG2_HPP_INCLUDED

#include <cstddef>

namespace foonathan
{
namespace tiny
{
    namespace detail
    {
        // undefined for 0
        template <typename UInt>
        constexpr bool is_power_of_two(UInt x) noexcept
        {
            return (x & (x - 1)) == 0;
        }

        constexpr std::size_t ilog2_base(std::size_t x) noexcept
        {
            return x == 0 ? 0 : ilog2_base(x >> 1) + 1;
        }

        // ilog2() implementation, cuts part after the comma
        // e.g. 1 -> 0, 2 -> 1, 3 -> 1, 4 -> 2, 5 -> 2
        constexpr std::size_t ilog2(std::size_t x) noexcept
        {
            return ilog2_base(x) - 1;
        }

        // ceiling ilog2() implementation, adds one if part after comma
        // e.g. 1 -> 0, 2 -> 1, 3 -> 2, 4 -> 2, 5 -> 3
        constexpr std::size_t ilog2_ceil(std::size_t x) noexcept
        {
            // only subtract one if power of two
            return ilog2_base(x) - std::size_t(is_power_of_two(x));
        }
    } // namespace detail
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_DETAIL_ILOG2_HPP_INCLUDED
