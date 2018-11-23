// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/tiny_storage.hpp>

#include <catch.hpp>

#include <foonathan/tiny/tiny_bool.hpp>
#include <foonathan/tiny/tiny_int.hpp>

using namespace foonathan::tiny;

TEST_CASE("tiny_storage")
{
    SECTION("basic")
    {
        struct first
        {};
        struct second
        {};
        using storage = tiny_storage<tiny_unsigned<7>, tiny_bool, tiny_bool>;

        storage     s;
        const auto& cs = s;

        REQUIRE(s.at<0>() == 0);
        REQUIRE(s.at<1>() == false);
        REQUIRE(s.at<2>() == false);

        s.at<0>() = 42;
        s.at<1>() = true;
        s.at<2>() = true;

        REQUIRE(s.at<0>() == 42);
        REQUIRE(s.at<1>() == true);
        REQUIRE(s.at<2>() == true);
        REQUIRE(cs.at<0>() == 42);
        REQUIRE(cs.at<1>() == true);
        REQUIRE(cs.at<2>() == true);

        REQUIRE(s[tiny_unsigned<7>{}] == 42);
        REQUIRE(cs[tiny_unsigned<7>{}] == 42);

        s = storage(7, false, true);
        REQUIRE(s.at<0>() == 7);
        REQUIRE(s.at<1>() == false);
        REQUIRE(s.at<2>() == true);
    }
    SECTION("empty")
    {
        using storage = tiny_storage<>;

        storage s;
        REQUIRE(s.spare_bits().size() == CHAR_BIT);
    }
}
