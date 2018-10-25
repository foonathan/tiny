// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/padding_tiny_storage.hpp>

#include <catch.hpp>

#include <foonathan/tiny/tiny_int.hpp>

using namespace foonathan::tiny;

namespace
{
struct no_padding
{
    std::uint8_t a;
    std::uint8_t b;
};

struct small_padding
{
    std::uint8_t  a;
    std::uint16_t b;
};

struct big_padding
{
    std::uint8_t  a;
    std::uint64_t b;
};

std::size_t object_count = 0;

struct non_trivial_padding
{
    std::uint8_t  a;
    std::uint16_t b;

    non_trivial_padding(std::uint8_t value) : a(value), b(value)
    {
        ++object_count;
    }

    non_trivial_padding(const non_trivial_padding& other) : a(other.a), b(other.b)
    {
        ++object_count;
    }

    ~non_trivial_padding()
    {
        --object_count;
    }
};
} // namespace

namespace foonathan
{
namespace tiny
{
    template <>
    struct padding_traits<small_padding>
    : padding_traits_aggregate<FOONATHAN_TINY_MEMBER(small_padding, a),
                               FOONATHAN_TINY_MEMBER(small_padding, b)>
    {};

    template <>
    struct padding_traits<big_padding>
    : padding_traits_aggregate<FOONATHAN_TINY_MEMBER(big_padding, a),
                               FOONATHAN_TINY_MEMBER(big_padding, b)>
    {};

    template <>
    struct padding_traits<non_trivial_padding>
    : padding_traits_aggregate<FOONATHAN_TINY_MEMBER(non_trivial_padding, a),
                               FOONATHAN_TINY_MEMBER(non_trivial_padding, b)>
    {};
} // namespace tiny
} // namespace foonathan

namespace
{
template <class Storage>
void verify_big_five(const Storage& s)
{
    Storage copy(s);
    REQUIRE(copy.object().a == s.object().a);
    REQUIRE(copy.object().b == s.object().b);
    REQUIRE(copy.tiny() == s.tiny());

    Storage move(std::move(copy));
    REQUIRE(move.object().a == s.object().a);
    REQUIRE(move.object().b == s.object().b);
    REQUIRE(move.tiny() == s.tiny());

    // move assign
    copy = Storage(s.object(), s.tiny() / 2);
    REQUIRE(copy.object().a == s.object().a);
    REQUIRE(copy.object().b == s.object().b);
    REQUIRE(copy.tiny() == s.tiny() / 2);

    // copy assign
    move = copy;
    REQUIRE(move.object().a == s.object().a);
    REQUIRE(move.object().b == s.object().b);
    REQUIRE(move.tiny() == s.tiny() / 2);
}
} // namespace

TEST_CASE("padding_tiny_storage")
{
    SECTION("no padding")
    {
        using storage = padding_tiny_storage<no_padding, tiny_unsigned<4>>;
        REQUIRE(!storage::is_compressed::value);

        storage s({0, 0});
        REQUIRE(s.object().a == 0);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.tiny() == 0);

        s.object().a = 42;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.tiny() == 0);

        s.tiny() = 15;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.tiny() == 15);

        s.object().b = 43;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 43);
        REQUIRE(s.tiny() == 15);

        verify_big_five(s);
    }
    SECTION("small padding")
    {
        using storage = padding_tiny_storage<small_padding, tiny_unsigned<4>>;
        REQUIRE(storage::is_compressed::value);

        storage s({0, 0});
        REQUIRE(s.object().a == 0);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.tiny() == 0);

        s.object().a = 42;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.tiny() == 0);

        s.tiny() = 15;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.tiny() == 15);

        s.object().b = 43;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 43);
        REQUIRE(s.tiny() == 15);

        verify_big_five(s);
    }
    SECTION("big padding")
    {
        using storage
            = padding_tiny_storage<big_padding, tiny_unsigned<8>, tiny_unsigned<48, std::uint64_t>>;
        REQUIRE(storage::is_compressed::value);

        storage s({0, 0});
        REQUIRE(s.object().a == 0);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.at<0>() == 0);
        REQUIRE(s.at<1>() == 0);

        s.object().a = 42;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.at<0>() == 0);
        REQUIRE(s.at<1>() == 0);

        s.at<0>() = 255;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.at<0>() == 255);
        REQUIRE(s.at<1>() == 0);

        s.object().b = 43;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 43);
        REQUIRE(s.at<0>() == 255);
        REQUIRE(s.at<1>() == 0);

        s.at<1>() = (1ull << 48) - 1;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 43);
        REQUIRE(s.at<0>() == 255);
        REQUIRE(s.at<1>() == (1ull << 48) - 1);
    }
    SECTION("non trivial padding")
    {
        using storage = padding_tiny_storage<non_trivial_padding, tiny_unsigned<4>>;
        REQUIRE(storage::is_compressed::value);

        storage s(0);
        REQUIRE(s.object().a == 0);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.tiny() == 0);

        s.object().a = 42;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.tiny() == 0);

        s.tiny() = 15;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 0);
        REQUIRE(s.tiny() == 15);

        s.object().b = 43;
        REQUIRE(s.object().a == 42);
        REQUIRE(s.object().b == 43);
        REQUIRE(s.tiny() == 15);

        verify_big_five(s);
    }
    SECTION("std::string")
    {
        using storage = padding_tiny_storage<std::string, tiny_unsigned<4>>;
        REQUIRE(!storage::is_compressed::value);

        storage s("hello!");
        REQUIRE(s.object() == "hello!");
        REQUIRE(s.tiny() == 0);

        s.object().pop_back();
        REQUIRE(s.object() == "hello");
        REQUIRE(s.tiny() == 0);

        s.tiny() = 15;
        REQUIRE(s.object() == "hello");
        REQUIRE(s.tiny() == 15);
    }
    REQUIRE(object_count == 0);
}
