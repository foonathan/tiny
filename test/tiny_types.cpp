// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/tiny_type.hpp>

#include <catch.hpp>

#include <foonathan/tiny/tiny_bool.hpp>
#include <foonathan/tiny/tiny_enum.hpp>
#include <foonathan/tiny/tiny_int.hpp>

using namespace foonathan::tiny;

namespace
{
using tiny_storage = int;

template <class TinyType>
typename TinyType::template proxy<bit_view<tiny_storage, 0, TinyType::bit_size()>> make_proxy(
    tiny_storage& storage)
{
    return make_tiny_proxy<TinyType>(make_bit_view<0, TinyType::bit_size()>(storage));
}
template <class TinyType>
typename TinyType::template proxy<bit_view<const tiny_storage, 0, TinyType::bit_size()>>
    make_cproxy(const tiny_storage& storage)
{
    return make_tiny_proxy<TinyType>(make_bit_view<0, TinyType::bit_size()>(storage));
}
} // namespace

namespace
{
template <class Proxy>
void verify_bool(Proxy proxy, bool value)
{
    if (value)
        REQUIRE(proxy);
    else
        REQUIRE(!proxy);

    REQUIRE(proxy == proxy);
    REQUIRE_FALSE(proxy != proxy);

    REQUIRE(proxy == value);
    REQUIRE(value == proxy);
    REQUIRE_FALSE(proxy != value);
    REQUIRE_FALSE(value != proxy);
}
} // namespace

TEST_CASE("tiny_bool")
{
    tiny_storage storage = 0;

    auto cproxy = make_cproxy<tiny_bool>(storage);
    verify_bool(cproxy, false);

    auto proxy = make_proxy<tiny_bool>(storage);
    verify_bool(proxy, false);

    proxy = true;
    verify_bool(proxy, true);
}

namespace
{
template <class Proxy, class Enum>
void verify_enum(Proxy proxy, Enum value)
{
    REQUIRE(static_cast<Enum>(proxy) == value);

    REQUIRE(proxy == proxy);
    REQUIRE_FALSE(proxy != proxy);

    REQUIRE(proxy == value);
    REQUIRE(value == proxy);
    REQUIRE_FALSE(proxy != value);
    REQUIRE_FALSE(value != proxy);
}
} // namespace

TEST_CASE("tiny_enum")
{
    enum class e
    {
        a,
        b,
        c,
        d,
        _unsigned_max = d,
    };
    using type           = tiny_enum<e>;
    tiny_storage storage = 0;

    auto cproxy = make_cproxy<type>(storage);
    verify_enum(cproxy, e::a);

    auto proxy = make_proxy<type>(storage);
    verify_enum(proxy, e::a);

    proxy = e::b;
    verify_enum(proxy, e::b);

    proxy = e::c;
    verify_enum(proxy, e::c);

    proxy = e::d;
    verify_enum(proxy, e::d);
}

namespace
{
template <class Proxy, class Int>
void verify_unsigned(Proxy proxy, Int value)
{
    REQUIRE(static_cast<Int>(proxy) == value);

    REQUIRE(proxy == proxy);
    REQUIRE_FALSE(proxy != proxy);

    REQUIRE(proxy == value);
    REQUIRE(value == proxy);
    REQUIRE_FALSE(proxy != value);
    REQUIRE_FALSE(value != proxy);

    REQUIRE(+proxy == value);
    REQUIRE(proxy + 0 == value + 0);
    REQUIRE(proxy - 0 == value);
    REQUIRE(proxy * 0 == value * 0);
    REQUIRE(proxy / 1 == value);
    REQUIRE(proxy % 1 == 0);
}
} // namespace

TEST_CASE("tiny_unsigned")
{
    using type           = tiny_unsigned<7>;
    tiny_storage storage = 0;

    auto cproxy = make_cproxy<type>(storage);
    verify_unsigned(cproxy, 0u);

    auto proxy = make_proxy<type>(storage);
    verify_unsigned(proxy, 0u);

    SECTION("all values")
    {
        for (auto i = 0u; i <= 127u; ++i)
        {
            proxy = i;
            verify_unsigned(proxy, i);
        }
    }
    SECTION("arithmetic operation")
    {
        proxy += 17;
        verify_unsigned(proxy, 17u);

        proxy -= 4;
        verify_unsigned(proxy, 13u);

        proxy *= 2;
        verify_unsigned(proxy, 26u);

        proxy /= 3;
        verify_unsigned(proxy, 8u);

        proxy %= 3;
        verify_unsigned(proxy, 2u);

        auto value = proxy++;
        verify_unsigned(proxy, 3u);
        REQUIRE(value == 2);

        value = ++proxy;
        verify_unsigned(proxy, 4u);
        REQUIRE(value == 4);

        value = proxy--;
        verify_unsigned(proxy, 3u);
        REQUIRE(value == 4);

        value = --proxy;
        verify_unsigned(proxy, 2u);
        REQUIRE(value == 2);
    }
}

namespace
{
template <class Proxy, class Int>
void verify_int(Proxy proxy, Int value)
{
    verify_unsigned(proxy, value);
    REQUIRE(-(-proxy) == value);
}
} // namespace

TEST_CASE("tiny_int")
{
    using type           = tiny_int<7>;
    tiny_storage storage = 0;

    auto cproxy = make_cproxy<type>(storage);
    verify_unsigned(cproxy, 0);

    auto proxy = make_proxy<type>(storage);
    verify_unsigned(proxy, 0);

    SECTION("all values")
    {
        for (auto i = -64; i <= 63; ++i)
        {
            proxy = i;
            verify_int(proxy, i);
        }
    }
    SECTION("arithmetic operation")
    {
        proxy += 13;
        verify_int(proxy, 13);

        proxy -= 17;
        verify_int(proxy, -4);

        proxy *= -4;
        verify_int(proxy, 16);

        proxy /= 3;
        verify_int(proxy, 5);

        proxy %= 2;
        verify_int(proxy, 1);
    }
}
