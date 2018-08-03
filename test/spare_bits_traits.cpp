// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/spare_bits.hpp>

#include <catch.hpp>

#include <foonathan/tiny/tiny_pair.hpp>
// use tiny_pair_impl for testing

using namespace foonathan::tiny;

namespace
{
    template <typename T>
    void verify_extract_put(T big)
    {
        tiny_pair_impl<T, spare_bits<T>()> pair(big, 0u);
        REQUIRE(decltype(pair)::is_compressed::value);
        REQUIRE(pair.integer() == 0u);
        REQUIRE(pair.big() == big);

        auto max_value = (static_cast<std::uintmax_t>(1) << spare_bits<T>()) - 1u;
        for (auto i = static_cast<typename decltype(pair)::integer_type>(0); i != max_value; ++i)
        {
            pair.set_integer(i);
            REQUIRE(pair.integer() == i);
            REQUIRE(pair.big() == big);
        }
    }

    template <typename T>
    void verify_clear(T big)
    {
        T t(big);
        if (spare_bits<T>() > 0u)
            put_spare_bits(t, 1u);
        clear_spare_bits(t);
        REQUIRE(t == big);
    }

    template <typename T>
    void verify_spare_bits(T big)
    {
        verify_extract_put(big);
        verify_clear(big);
    }
} // namespace

TEST_CASE("spare_bits_traits default")
{
    REQUIRE(spare_bits<std::string>() == 0u);
    verify_spare_bits(std::string("hello world"));
}

TEST_CASE("spare_bits_traits bool")
{
    REQUIRE(spare_bits<bool>() == 7);
    verify_spare_bits(true);
    verify_spare_bits(false);
}

namespace
{
    template <typename T>
    void verify_pointer(std::size_t align, std::size_t spare)
    {
        REQUIRE(alignof(T) == align);
        REQUIRE(spare_bits<T*>() == spare);

        T obj;
        verify_spare_bits(&obj);
        verify_spare_bits(static_cast<T*>(nullptr));
    }
} // namespace

TEST_CASE("spare_bits_traits pointer")
{
    SECTION("min align == ?")
    {
        REQUIRE(spare_bits<void*>() == 0u);
        REQUIRE(spare_bits<const void*>() == 0u);
        REQUIRE(spare_bits<volatile void*>() == 0u);
        REQUIRE(spare_bits<const volatile void*>() == 0u);
    }
    SECTION("min align == 1")
    {
        REQUIRE(spare_bits<char*>() == 0u);
        REQUIRE(spare_bits<const unsigned char*>() == 0u);
    }
    SECTION("min align > 1")
    {
        verify_pointer<std::uint16_t>(2, 1);
        verify_pointer<std::uint32_t>(4, 2);
        verify_pointer<std::uint64_t>(8, 3);
    }
}
