// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/tiny_type.hpp>

#include <catch.hpp>

#include <foonathan/tiny/tiny_bool.hpp>
#include <foonathan/tiny/tiny_enum.hpp>
#include <foonathan/tiny/tiny_flag_set.hpp>
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

namespace
{
enum class test_flags
{
    a,
    b,
    c,

    flag_count_,
};

template <class Proxy>
void verify_flag_set(Proxy proxy, bool a_set, bool b_set, bool c_set)
{
    REQUIRE(proxy[test_flags::a] == a_set);
    REQUIRE(proxy[test_flags::b] == b_set);
    REQUIRE(proxy[test_flags::c] == c_set);

    REQUIRE(proxy.get() == std::uintmax_t((c_set << 2) | (b_set << 1) | (a_set << 0)));

    REQUIRE(proxy.is_set(test_flags::a) == a_set);
    REQUIRE(proxy.is_set(test_flags::b) == b_set);
    REQUIRE(proxy.is_set(test_flags::c) == c_set);

    REQUIRE(proxy.any() == (a_set || b_set || c_set));
    REQUIRE(proxy.all() == (a_set && b_set && c_set));
    REQUIRE(proxy.none() == (!a_set && !b_set && !c_set));

    REQUIRE(proxy == proxy);
    REQUIRE_FALSE(proxy != proxy);

    if (proxy.any())
    {
        REQUIRE(proxy != flags<test_flags>());
        REQUIRE(flags<test_flags>() != proxy);
        REQUIRE_FALSE(proxy == flags<test_flags>());
        REQUIRE_FALSE(flags<test_flags>() == proxy);
    }
    else
    {
        REQUIRE(proxy == flags<test_flags>());
        REQUIRE(flags<test_flags>() == proxy);
        REQUIRE_FALSE(proxy != flags<test_flags>());
        REQUIRE_FALSE(flags<test_flags>() != proxy);
    }
}
} // namespace

TEST_CASE("tiny_flag_set")
{
    tiny_storage storage = 0;

    auto cproxy = make_cproxy<tiny_flag_set<test_flags>>(storage);
    verify_flag_set(cproxy, false, false, false);

    auto proxy = make_proxy<tiny_flag_set<test_flags>>(storage);
    verify_flag_set(proxy, false, false, false);

    proxy = flags(test_flags::a, test_flags::c);
    verify_flag_set(proxy, true, false, true);

    SECTION("single flag operation")
    {
        proxy[test_flags::b] = true;
        verify_flag_set(proxy, true, true, true);

        proxy.reset(test_flags::a);
        verify_flag_set(proxy, false, true, true);

        proxy.set(test_flags::a);
        verify_flag_set(proxy, true, true, true);

        proxy.toggle(test_flags::a);
        verify_flag_set(proxy, false, true, true);

        proxy.toggle(test_flags::a);
        verify_flag_set(proxy, true, true, true);
    }
    SECTION("multi flag operation")
    {
        proxy.toggle_all();
        verify_flag_set(proxy, false, true, false);

        proxy.toggle_all();
        verify_flag_set(proxy, true, false, true);

        SECTION("set_all")
        {
            proxy.set_all();
            verify_flag_set(proxy, true, true, true);
        }
        SECTION("reset_all")
        {
            proxy.reset_all();
            verify_flag_set(proxy, false, false, false);
        }
    }
}
