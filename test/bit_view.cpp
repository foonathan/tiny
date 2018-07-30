// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/bit_view.hpp>

#include <catch.hpp>

using namespace foonathan::tiny;

namespace
{
    using test_integer = std::uint32_t;

    template <std::size_t Begin, std::size_t End>
    void test_bit_view(bit_view<test_integer, Begin, End> view, std::uintmax_t value,
                       const std::string& str)
    {
        REQUIRE(view.begin() == Begin);
        REQUIRE(view.end() == End);
        REQUIRE(view.size() == str.size());

        std::string stored_str;
        for (auto i = 0u; i != view.size(); ++i)
            if (view[i])
                stored_str += '1';
            else
                stored_str += '0';
        REQUIRE(stored_str == str);

        REQUIRE(view.extract() == value);
    }
} // namespace

TEST_CASE("bit_view")
{
    test_integer integer = 0u;

    SECTION("basic")
    {
        bit_view<test_integer, 0, 10> view(integer);
        test_bit_view(view, 0u, "0000000000");
        REQUIRE(integer == 0u);

        integer = 0xFF;
        test_bit_view(view, 0xFF, "1111111100");
        REQUIRE(integer == 0xFF);

        integer += 2048;
        test_bit_view(view, 0xFF, "1111111100");

        view[0] = false;
        test_bit_view(view, 0xFE, "0111111100");
        REQUIRE(integer == 2048 + 0xFE);

        view.put(0xAA);
        test_bit_view(view, 0xAA, "0101010100");
        REQUIRE(integer == 2048 + 0xAA);
    }
    SECTION("no modification outside")
    {
        integer = test_integer(-1);
        bit_view<test_integer, 0, 4> view(integer);
        test_bit_view(view, 15, "1111");

        SECTION("put")
        {
            view.put(0);
            test_bit_view(view, 0, "0000");
            REQUIRE(integer == test_integer(-1) - 15);
        }
        SECTION("operator[]")
        {
            view[0] = false;
            view[1] = false;
            view[2] = false;
            view[3] = false;
            test_bit_view(view, 0, "0000");
            REQUIRE(integer == test_integer(-1) - 15);
        }
    }
    SECTION("align middle")
    {
        bit_view<test_integer, 4, 8> view(integer);
        test_bit_view(view, 0, "0000");

        SECTION("put")
        {
            view.put(15);
            test_bit_view(view, 15, "1111");
            REQUIRE(integer == 15u << 4);
        }
        SECTION("operator[]")
        {
            view[0] = true;
            view[1] = true;
            view[2] = true;
            view[3] = true;
            test_bit_view(view, 15, "1111");
            REQUIRE(integer == 15u << 4);
        }
    }
    SECTION("align end")
    {
        integer = test_integer(-1);
        bit_view<test_integer, 16, 32> view(integer);
        test_bit_view(view, UINT16_MAX, "1111111111111111");

        SECTION("put")
        {
            view.put(0);
            test_bit_view(view, 0, "0000000000000000");
            integer = UINT16_MAX;
        }
        SECTION("operator[]")
        {
            for (auto i = 0u; i != 16u; ++i)
                view[i] = false;
            test_bit_view(view, 0, "0000000000000000");
            integer = UINT16_MAX;
        }
    }
    SECTION("whole")
    {
        bit_view<test_integer, 0, 32> view(integer);
        view.put(std::uintmax_t(-1));
        REQUIRE(integer == UINT32_MAX);
    }
}
