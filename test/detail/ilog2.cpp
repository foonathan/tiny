// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/detail/ilog2.hpp>

#include <catch.hpp>

using namespace foonathan::tiny::detail;

namespace
{
void check(std::size_t x, std::size_t a, std::size_t b)
{
    INFO(x);
    CHECK(ilog2(x) == a);
    REQUIRE(ilog2_ceil(x) == b);
}
} // namespace

TEST_CASE("detail::ilog2")
{
    check(1, 0, 0);
    check(2, 1, 1);
    check(3, 1, 2);
    check(4, 2, 2);
    check(5, 2, 3);
    check(6, 2, 3);
    check(7, 2, 3);
    check(8, 3, 3);

    check(255, 7, 8);
    check(256, 8, 8);
}
