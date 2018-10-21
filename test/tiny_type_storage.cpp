// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/tiny_type_storage.hpp>

#include <catch.hpp>

using namespace foonathan::tiny;

TEST_CASE("tiny_type_storage")
{
    struct first
    {};
    struct second
    {};
    using storage = tiny_type_storage<tiny_unsigned<7>, tiny_tagged<first, tiny_bool>,
                                      tiny_tagged<second, tiny_bool>>;

    storage     s;
    const auto& cs = s;

    REQUIRE(s.get<0>() == 0);
    REQUIRE(s.get<1>() == false);
    REQUIRE(s.get<2>() == false);

    s.get<0>() = 42;
    s.get<1>() = true;
    s.get<2>() = true;

    REQUIRE(s.get<0>() == 42);
    REQUIRE(s.get<1>() == true);
    REQUIRE(s.get<2>() == true);
    REQUIRE(cs.get<0>() == 42);
    REQUIRE(cs.get<1>() == true);
    REQUIRE(cs.get<2>() == true);

    REQUIRE(s[tiny_unsigned<7>{}] == 42);
    REQUIRE(s[first{}] == true);
    REQUIRE(s[second{}] == true);
    REQUIRE(cs[tiny_unsigned<7>{}] == 42);
    REQUIRE(cs[first{}] == true);
    REQUIRE(cs[second{}] == true);

    REQUIRE(s.get<tiny_unsigned<7>>() == 42);
    REQUIRE(s.get<first>() == true);
    REQUIRE(s.get<second>() == true);
    REQUIRE(cs.get<tiny_unsigned<7>>() == 42);
    REQUIRE(cs.get<first>() == true);
    REQUIRE(cs.get<second>() == true);

    s = storage(7, false, true);
    REQUIRE(s.get<0>() == 7);
    REQUIRE(s.get<1>() == false);
    REQUIRE(s.get<2>() == true);
}
