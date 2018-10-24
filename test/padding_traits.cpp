// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/padding_traits.hpp>

#include <catch.hpp>
#include <cstring>

using namespace foonathan::tiny;

TEST_CASE("aggregate_member")
{
    struct foo
    {
        int       a;
        const int b;
        char      c;
    };

    foo       f{1, 2, 'a'};
    const foo cf{1, 2, 'a'};

    using member_a = FOONATHAN_TINY_MEMBER(foo, a);
    REQUIRE(std::is_same<member_a::object_type, foo>::value);
    REQUIRE(std::is_same<member_a::member_type, int>::value);
    REQUIRE(member_a::offset() == 0);

    REQUIRE(member_a::get(cf) == 1);

    member_a::get(f) = -1;
    REQUIRE(member_a::get(f) == -1);

    using member_b = FOONATHAN_TINY_MEMBER(foo, b);
    REQUIRE(std::is_same<member_b::object_type, foo>::value);
    REQUIRE(std::is_same<member_b::member_type, const int>::value);
    REQUIRE(member_b::offset() == sizeof(int));

    REQUIRE(member_b::get(f) == 2);
    REQUIRE(member_b::get(cf) == 2);

    using member_c = FOONATHAN_TINY_MEMBER(foo, c);
    REQUIRE(std::is_same<member_c::object_type, foo>::value);
    REQUIRE(std::is_same<member_c::member_type, char>::value);
    REQUIRE(member_c::offset() == 2 * sizeof(int));

    REQUIRE(member_c::get(cf) == 'a');

    member_c::get(f) = 'A';
    REQUIRE(member_c::get(f) == 'A');
}

TEST_CASE("padding_traits_aggregate")
{
    SECTION("no padding")
    {
        struct foo
        {
            std::uint8_t a;
            std::uint8_t b;
        };

        using traits = padding_traits_aggregate<FOONATHAN_TINY_MEMBER(foo, a),
                                                FOONATHAN_TINY_MEMBER(foo, b)>;

        foo                                        obj;
        bit_view<unsigned char[sizeof(foo)], 0, 0> padding = traits::padding_view(obj);
        REQUIRE(padding.size() == 0);
    }
    SECTION("single padding")
    {
        struct foo
        {
            std::uint8_t  a;
            std::uint16_t b;
        };

        using traits = padding_traits_aggregate<FOONATHAN_TINY_MEMBER(foo, a),
                                                FOONATHAN_TINY_MEMBER(foo, b)>;

        foo  obj;
        auto byte_ptr = reinterpret_cast<unsigned char*>(&obj);
        std::memset(&obj, 0, sizeof(foo));
        REQUIRE(obj.a == 0);
        REQUIRE(obj.b == 0);

        bit_view<unsigned char[sizeof(foo)], 8, 16> padding = traits::padding_view(obj);
        REQUIRE(padding.size() == 8);
        REQUIRE(padding.extract() == 0);

        padding.put(~0ull);
        REQUIRE(obj.a == 0);
        REQUIRE(obj.b == 0);
        REQUIRE(byte_ptr[1] == 0xFF);
    }
    SECTION("complex")
    {
        struct foo
        {
            std::uint8_t  a;
            std::uint32_t b;
            std::uint64_t c;
            std::uint8_t  d;
        };

        using traits
            = padding_traits_aggregate<FOONATHAN_TINY_MEMBER(foo, a), FOONATHAN_TINY_MEMBER(foo, b),
                                       FOONATHAN_TINY_MEMBER(foo, c),
                                       FOONATHAN_TINY_MEMBER(foo, d)>;

        foo  obj;
        auto byte_ptr = reinterpret_cast<unsigned char*>(&obj);
        std::memset(&obj, 0, sizeof(foo));
        REQUIRE(obj.a == 0);
        REQUIRE(obj.b == 0);
        REQUIRE(obj.c == 0);
        REQUIRE(obj.d == 0);

        joined_bit_view<bit_view<unsigned char[sizeof(foo)], 8, 32>,
                        bit_view<unsigned char[sizeof(foo)], 136, sizeof(foo)* CHAR_BIT>>
            padding = traits::padding_view(obj);
        REQUIRE(padding.size() == (3 + 0 + 0 + 7) * CHAR_BIT);
        REQUIRE(padding.subview<0, 3 * CHAR_BIT>().extract() == 0);
        REQUIRE(padding.subview<3 * CHAR_BIT, last_bit>().extract() == 0);

        padding.subview<0, 3 * CHAR_BIT>().put(~0ull);
        REQUIRE(obj.a == 0);
        REQUIRE(obj.b == 0);
        REQUIRE(obj.c == 0);
        REQUIRE(obj.d == 0);
        REQUIRE(byte_ptr[1] == 0xFF);
        REQUIRE(byte_ptr[2] == 0xFF);
        REQUIRE(byte_ptr[3] == 0xFF);

        padding.subview<3 * CHAR_BIT, last_bit>().put(~0ull);
        REQUIRE(obj.a == 0);
        REQUIRE(obj.b == 0);
        REQUIRE(obj.c == 0);
        REQUIRE(obj.d == 0);
        for (auto i = 0; i != 7; ++i)
            REQUIRE(byte_ptr[2 + 2 * 64 / CHAR_BIT] == 0xFF);

        const auto& cobj     = obj;
        auto        cpadding = traits::padding_view(cobj);
        cpadding             = padding;
    }
}
