// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/bit_field.hpp>

#include <catch.hpp>

namespace tiny = foonathan::tiny;

namespace
{
template <class Reference>
void verify_bool(Reference reference, bool value)
{
    REQUIRE(reference.bits() == 1);

    REQUIRE(reference.value() == value);

    REQUIRE(reference == reference);
    REQUIRE_FALSE(reference != reference);

    REQUIRE(reference == value);
    REQUIRE(value == reference);
    REQUIRE_FALSE(reference != value);
    REQUIRE_FALSE(value != reference);

    if (value)
        REQUIRE(reference);
    else
        REQUIRE(!reference);
}
} // namespace

TEST_CASE("bit_field_bool")
{
    using first  = tiny::bit_field_bool<struct first_tag>;
    using second = tiny::bit_field_bool<struct second_tag>;

    using bit_fields = tiny::bit_fields<first, second>;
    REQUIRE(sizeof(bit_fields) == 1);

    bit_fields fields{};
    verify_bool(fields[first{}], false);
    verify_bool(fields[second{}], false);

    fields[first{}] = true;
    verify_bool(fields[first{}], true);
    verify_bool(fields[second{}], false);

    fields[second{}] = true;
    verify_bool(fields[first{}], true);
    verify_bool(fields[second{}], true);

    const auto& cfields = fields;
    verify_bool(cfields[first{}], true);
    verify_bool(cfields[second{}], true);
}

namespace
{
template <std::size_t Bits, class Reference, class Enum>
void verify_enum(Reference reference, Enum value)
{
    REQUIRE(reference.bits() == Bits);

    REQUIRE(reference.value() == value);
    REQUIRE(static_cast<Enum>(reference) == value);

    REQUIRE(reference == reference);
    REQUIRE_FALSE(reference != reference);

    REQUIRE(reference == value);
    REQUIRE(value == reference);
    REQUIRE_FALSE(reference != value);
    REQUIRE_FALSE(value != reference);
}
} // namespace

TEST_CASE("bit_field_enum")
{
    enum class enum1
    {
        a,
        b,
        c,
        d,
    };
    using first  = tiny::bit_field_enum<struct first_tag, enum1, enum1::d>;
    using second = tiny::bit_field_enum<struct second_tag, enum1, enum1::b>;

    enum class enum2
    {
        a,
        b,
        c,
        d,
    };
    using third = tiny::bit_field_enum<struct third_tag, enum2, enum2::d>;

    using bit_fields = tiny::bit_fields<first, second, third>;
    REQUIRE(sizeof(bit_fields) == 1);

    bit_fields fields{};
    verify_enum<2>(fields[first{}], enum1::a);
    verify_enum<1>(fields[second{}], enum1::a);
    verify_enum<2>(fields[third{}], enum2::a);

    fields[first{}] = enum1::d;
    verify_enum<2>(fields[first{}], enum1::d);
    verify_enum<1>(fields[second{}], enum1::a);
    verify_enum<2>(fields[third{}], enum2::a);

    fields[third{}] = enum2::c;
    verify_enum<2>(fields[first{}], enum1::d);
    verify_enum<1>(fields[second{}], enum1::a);
    verify_enum<2>(fields[third{}], enum2::c);

    const auto& cfields = fields;
    verify_enum<2>(cfields[first{}], enum1::d);
    verify_enum<1>(cfields[second{}], enum1::a);
    verify_enum<2>(cfields[third{}], enum2::c);
}

namespace
{
template <std::size_t Bits, class Reference, class Int>
void verify_unsigned(Reference reference, Int value)
{
    REQUIRE(reference.bits() == Bits);

    REQUIRE(reference.value() == value);
    REQUIRE(static_cast<Int>(reference) == value);

    REQUIRE(reference == reference);
    REQUIRE_FALSE(reference != reference);

    REQUIRE(reference == value);
    REQUIRE(value == reference);
    REQUIRE_FALSE(reference != value);
    REQUIRE_FALSE(value != reference);
}
} // namespace

TEST_CASE("bit_field_unsigned")
{
    using first = tiny::bit_field_unsigned<struct first_tag, 2>;
    REQUIRE(first::max() == 3);
    using second = tiny::bit_field_unsigned<struct second_tag, 8>;
    REQUIRE(second::max() == 255);
    using third = tiny::bit_field_unsigned<struct third_tag, 4>;
    REQUIRE(third::max() == 15);

    using bit_fields = tiny::bit_fields<first, second, third>;
    REQUIRE(sizeof(bit_fields) == 2);

    bit_fields fields{};
    verify_unsigned<2>(fields[first{}], 0);
    verify_unsigned<8>(fields[second{}], 0);
    verify_unsigned<4>(fields[third{}], 0);

    fields[first{}] = 3;
    verify_unsigned<2>(fields[first{}], 3);
    verify_unsigned<8>(fields[second{}], 0);
    verify_unsigned<4>(fields[third{}], 0);

    fields[second{}] = 42;
    verify_unsigned<2>(fields[first{}], 3);
    verify_unsigned<8>(fields[second{}], 42);
    verify_unsigned<4>(fields[third{}], 0);

    fields[third{}] = 15;
    verify_unsigned<2>(fields[first{}], 3);
    verify_unsigned<8>(fields[second{}], 42);
    verify_unsigned<4>(fields[third{}], 15);

    const auto& cfields = fields;
    verify_unsigned<2>(cfields[first{}], 3);
    verify_unsigned<8>(cfields[second{}], 42);
    verify_unsigned<4>(cfields[third{}], 15);

    SECTION("arithmetic operators")
    {
        fields[second{}] = 0;

        fields[second{}] += 17;
        REQUIRE(fields[second{}] == 17);

        fields[second{}] -= 4;
        REQUIRE(fields[second{}] == 13);

        fields[second{}] *= 2;
        REQUIRE(fields[second{}] == 26);

        fields[second{}] /= 4;
        REQUIRE(fields[second{}] == 6);

        fields[second{}] %= 3;
        REQUIRE(fields[second{}] == 0);

        int value = fields[second{}]++;
        REQUIRE(fields[second{}] == 1);
        REQUIRE(value == 0);

        value = ++fields[second{}];
        REQUIRE(fields[second{}] == 2);
        REQUIRE(value == 2);

        value = fields[second{}]--;
        REQUIRE(fields[second{}] == 1);
        REQUIRE(value == 2);

        value = --fields[second{}];
        REQUIRE(fields[second{}] == 0);
        REQUIRE(value == 0);
    }
}

namespace
{
template <std::size_t Bits, class Reference, class Int>
void verify_signed(Reference reference, Int value)
{
    REQUIRE(reference.bits() == Bits);

    REQUIRE(reference.value() == value);
    REQUIRE(static_cast<Int>(reference) == value);

    REQUIRE(reference == reference);
    REQUIRE_FALSE(reference != reference);

    REQUIRE(reference == value);
    REQUIRE(value == reference);
    REQUIRE_FALSE(reference != value);
    REQUIRE_FALSE(value != reference);
}
} // namespace

TEST_CASE("bit_field_signed")
{
    using first = tiny::bit_field_signed<struct first_tag, 2>;
    REQUIRE(first::min() == -2);
    REQUIRE(first::max() == 1);
    using second = tiny::bit_field_signed<struct second_tag, 8>;
    REQUIRE(second::min() == -128);
    REQUIRE(second::max() == 127);
    using third = tiny::bit_field_signed<struct third_tag, 4>;
    REQUIRE(third::min() == -8);
    REQUIRE(third::max() == 7);

    using bit_fields = tiny::bit_fields<first, second, third>;
    REQUIRE(sizeof(bit_fields) == 2);

    bit_fields fields{};
    verify_signed<2>(fields[first{}], 0);
    verify_signed<8>(fields[second{}], 0);
    verify_signed<4>(fields[third{}], 0);

    fields[first{}] = 1;
    verify_signed<2>(fields[first{}], 1);
    verify_signed<8>(fields[second{}], 0);
    verify_signed<4>(fields[third{}], 0);

    fields[second{}] = 42;
    verify_signed<2>(fields[first{}], 1);
    verify_signed<8>(fields[second{}], 42);
    verify_signed<4>(fields[third{}], 0);

    fields[third{}] = -8;
    verify_signed<2>(fields[first{}], 1);
    verify_signed<8>(fields[second{}], 42);
    verify_signed<4>(fields[third{}], -8);

    const auto& cfields = fields;
    verify_signed<2>(cfields[first{}], 1);
    verify_signed<8>(cfields[second{}], 42);
    verify_signed<4>(cfields[third{}], -8);

    SECTION("arithmetic operators")
    {
        fields[second{}] = 0;

        fields[second{}] += 17;
        REQUIRE(fields[second{}] == 17);

        fields[second{}] -= 4;
        REQUIRE(fields[second{}] == 13);

        fields[second{}] *= 2;
        REQUIRE(fields[second{}] == 26);

        fields[second{}] /= 4;
        REQUIRE(fields[second{}] == 6);

        fields[second{}] %= 3;
        REQUIRE(fields[second{}] == 0);

        int value = fields[second{}]++;
        REQUIRE(fields[second{}] == 1);
        REQUIRE(value == 0);

        value = ++fields[second{}];
        REQUIRE(fields[second{}] == 2);
        REQUIRE(value == 2);

        value = fields[second{}]--;
        REQUIRE(fields[second{}] == 1);
        REQUIRE(value == 2);

        value = --fields[second{}];
        REQUIRE(fields[second{}] == 0);
        REQUIRE(value == 0);
    }
}
