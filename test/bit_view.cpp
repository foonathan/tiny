// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/bit_view.hpp>

#include <catch.hpp>

using namespace foonathan::tiny;

namespace
{
using test_integer = std::uint32_t;

template <typename T, std::size_t Begin, std::size_t End>
void test_bit_view(bit_view<T, Begin, End> view, std::uintmax_t value, const std::string& str)
{
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

        SECTION("subview")
        {
            auto sub = view.subview<2, 6>();
            test_bit_view(sub, 0xA, "0101");
        }
    }
    SECTION("no modification outside")
    {
        integer = UINT32_MAX;
        bit_view<test_integer, 0, 4> view(integer);
        test_bit_view(view, 15, "1111");

        SECTION("put")
        {
            view.put(0);
            test_bit_view(view, 0, "0000");
            REQUIRE(integer == 0xFFFFFFF0);
        }
        SECTION("operator[]")
        {
            view[0] = false;
            view[1] = false;
            view[2] = false;
            view[3] = false;
            test_bit_view(view, 0, "0000");
            REQUIRE(integer == 0xFFFFFFF0);
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
        integer = UINT32_MAX;
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

TEST_CASE("bit_view array")
{
    test_integer array[3] = {0u, 0u, 0u};

    SECTION("basic")
    {
        SECTION("two array elements")
        {
            bit_view<test_integer[3], 24, 40> view(array);
            test_bit_view(view, 0u, "0000000000000000");
            REQUIRE(array[0] == 0u);
            REQUIRE(array[1] == 0u);
            REQUIRE(array[2] == 0u);

            array[0] = 0xFF000000;
            array[1] = 0xC;
            test_bit_view(view, 0xCFF, "1111111100110000");
            REQUIRE(array[0] == 0xFF000000);
            REQUIRE(array[1] == 0xC);
            REQUIRE(array[2] == 0u);

            array[0] |= 0xFF0000;
            array[1] |= 0xFF00;
            test_bit_view(view, 0xCFF, "1111111100110000");

            view[0] = false;
            test_bit_view(view, 0xCFE, "0111111100110000");
            REQUIRE(array[0] == 0xFEFF0000);
            REQUIRE(array[1] == 0xFF0C);
            REQUIRE(array[2] == 0u);

            view.put(0xAAAA);
            test_bit_view(view, 0xAAAA, "0101010101010101");
            REQUIRE(array[0] == 0xAAFF0000);
            REQUIRE(array[1] == 0xFFAA);
            REQUIRE(array[2] == 0u);

            SECTION("subview")
            {
                auto sub = view.subview<6, 10>();
                test_bit_view(sub, 0xA, "0101");
            }
        }
        SECTION("three array elements")
        {
            bit_view<test_integer[3], 30, 66> view(array);
            test_bit_view(view, 0u, "000000000000000000000000000000000000");
            REQUIRE(array[0] == 0u);
            REQUIRE(array[1] == 0u);
            REQUIRE(array[2] == 0u);

            array[0] = 0x80000000;
            array[1] = 0xAAAAAAAA;
            array[2] = 0xF2;
            test_bit_view(view, 0xAAAAAAAAA, "010101010101010101010101010101010101");

            array[1] |= 0x55555555;
            test_bit_view(view, 0xBFFFFFFFE, "011111111111111111111111111111111101");

            view[0] = true;
            test_bit_view(view, 0xBFFFFFFFF, "111111111111111111111111111111111101");
            REQUIRE(array[0] == 0xC0000000);
            REQUIRE(array[1] == 0xFFFFFFFF);
            REQUIRE(array[2] == 0xF2);

            view.put(0x5555);
            test_bit_view(view, 0x5555, "101010101010101000000000000000000000");
            REQUIRE(array[0] == 0x40000000);
            REQUIRE(array[1] == 0x1555);
            REQUIRE(array[2] == 0xF0);

            SECTION("subview")
            {
                auto sub = view.subview<0, 4>();
                test_bit_view(sub, 0x5, "1010");
            }
        }
    }
    SECTION("no modification outside")
    {
        array[0] = UINT32_MAX;
        array[1] = UINT32_MAX;
        array[2] = UINT32_MAX;

        SECTION("one array element")
        {
            bit_view<test_integer[3], 36, 44> view(array);
            test_bit_view(view, 0xFF, "11111111");

            SECTION("put")
            {
                view.put(0);
                test_bit_view(view, 0, "00000000");
                REQUIRE(array[0] == UINT32_MAX);
                REQUIRE(array[1] == 0xFFFFF00F);
                REQUIRE(array[2] == UINT32_MAX);
            }
            SECTION("operator[]")
            {
                view[0] = false;
                view[1] = false;
                view[2] = false;
                view[3] = false;
                test_bit_view(view, 0xF0, "00001111");
                REQUIRE(array[0] == UINT32_MAX);
                REQUIRE(array[1] == 0xFFFFFF0F);
                REQUIRE(array[2] == UINT32_MAX);
            }
        }
        SECTION("two array elements")
        {
            bit_view<test_integer[3], 30, 34> view(array);
            test_bit_view(view, 15, "1111");

            SECTION("put")
            {
                view.put(0);
                test_bit_view(view, 0, "0000");
                REQUIRE(array[0] == 0x3FFFFFFF);
                REQUIRE(array[1] == 0xFFFFFFFC);
                REQUIRE(array[2] == UINT32_MAX);
            }
            SECTION("operator[]")
            {
                view[0] = false;
                view[1] = false;
                view[2] = false;
                view[3] = false;
                test_bit_view(view, 0, "0000");
                REQUIRE(array[0] == 0x3FFFFFFF);
                REQUIRE(array[1] == 0xFFFFFFFC);
                REQUIRE(array[2] == UINT32_MAX);
            }
        }
        SECTION("three array elements")
        {
            bit_view<test_integer[3], 28, 68> view(array);
            test_bit_view(view, 0xFFFFFFFFFF, "1111111111111111111111111111111111111111");

            SECTION("put")
            {
                view.put(0);
                test_bit_view(view, 0, "0000000000000000000000000000000000000000");
                REQUIRE(array[0] == 0x0FFFFFFF);
                REQUIRE(array[1] == 0x00000000);
                REQUIRE(array[2] == 0xFFFFFFF0);
            }
            SECTION("operator[]")
            {
                view[0]  = false;
                view[1]  = false;
                view[2]  = false;
                view[3]  = false;
                view[8]  = false;
                view[9]  = false;
                view[10] = false;
                view[11] = false;
                view[36] = false;
                view[37] = false;
                view[38] = false;
                view[39] = false;
                test_bit_view(view, 0x0FFFFFF0F0, "0000111100001111111111111111111111110000");
                REQUIRE(array[0] == 0x0FFFFFFF);
                REQUIRE(array[1] == 0xFFFFFF0F);
                REQUIRE(array[2] == 0xFFFFFFF0);
            }
        }
    }
}

TEST_CASE("joined_bit_view")
{
    int array[2] = {0, 0};
    int first    = 0;
    int second   = 0;

    using view_t
        = joined_bit_view<bit_view<int, 0, 4>, bit_view<int[2], 0, 4>, bit_view<int, 0, 4>>;
    using cview_t
        = joined_bit_view<bit_view<const int, 0, 4>, bit_view<int[2], 0, 4>, bit_view<int, 0, 4>>;

    view_t view(first, array, second);
    test_bit_view(view, 0, "000000000000");

    cview_t cview(view);
    test_bit_view(cview, 0, "000000000000");

    first = 0xFF;
    test_bit_view(view, 0xF, "111100000000");

    view.put(0x7F3);
    test_bit_view(view, 0x7F3, "110011111110");
    REQUIRE(first == 0xF3);
    REQUIRE(array[0] == 0xF);
    REQUIRE(second == 0x7);

    view = join_bit_views(make_bit_view<0, 4>(second), make_bit_view<0, 4>(array),
                          make_bit_view<0, 4>(first));
    test_bit_view(view, 0x3F7, "111011111100");

    view[3] = true;
    test_bit_view(view, 0x3FF, "111111111100");

    bit_view<int, 0, 4> sub_first = view.subview<0, 4>();
    test_bit_view(sub_first, 0xF, "1111");

    bit_view<int[2], 0, 4> sub_second = view.subview<4, 8>();
    test_bit_view(sub_second, 0xF, "1111");

    bit_view<int, 0, 4> sub_third = view.subview<8, 12>();
    test_bit_view(sub_third, 0x3, "1100");

    bit_view<int, 0, 2> sub_sub_first = view.subview<0, 2>();
    test_bit_view(sub_sub_first, 0x3, "11");

    auto second_third = view.subview<4, 10>();
    test_bit_view(second_third, 0x3F, "111111");
}

TEST_CASE("bit_view convenience")
{
    unsigned value = 0;

    put_bits<0, 1>(value, 1);
    REQUIRE(value == 1);

    put_bits<1, 3>(value, 7);
    REQUIRE(value == 7);

    REQUIRE(extract_bits<1, 3>(value) == 3);

    REQUIRE(are_cleared_bits<3, 5>(value));
    REQUIRE(are_only_bits<0, 3>(value));
    REQUIRE(are_only_bits<0, 5>(value));
    REQUIRE(are_only_bits<0, last_bit>(value));

    clear_bits<1, 2>(value);
    REQUIRE(value == 5);

    clear_other_bits<1, 2>(value);
    REQUIRE(value == 0);
}
