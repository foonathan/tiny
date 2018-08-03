// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/tiny_pair.hpp>

#include <catch.hpp>

using namespace foonathan::tiny;

namespace
{
    struct spare_test_type
    {
        std::uint16_t spare_bits;

        spare_test_type() : spare_bits(UINT16_MAX) {}

        spare_test_type(const spare_test_type&) : spare_bits(UINT16_MAX) {}

        ~spare_test_type() noexcept
        {
            verify();
        }

        spare_test_type& operator=(const spare_test_type& other) noexcept
        {
            verify();
            other.verify();
            return *this;
        }

        void verify() const
        {
            REQUIRE(spare_bits == UINT16_MAX);
        }

        friend bool operator==(const spare_test_type&, const spare_test_type&) noexcept
        {
            return true;
        }
        friend bool operator<(const spare_test_type&, const spare_test_type&) noexcept
        {
            return false;
        }
    };
} // namespace

namespace foonathan
{
    namespace tiny
    {
        template <>
        struct spare_bits_traits<spare_test_type>
        {
            static constexpr std::size_t spare_bits = 16u;

            static void clear(spare_test_type& obj) noexcept
            {
                put(obj, UINT16_MAX);
            }

            static std::uintmax_t extract(const spare_test_type& obj) noexcept
            {
                return obj.spare_bits;
            }

            static void put(spare_test_type& obj, std::uintmax_t spare_bits) noexcept
            {
                obj.spare_bits = static_cast<std::uint16_t>(spare_bits);
            }
        };
    } // namespace tiny
} // namespace foonathan

namespace
{
    template <std::size_t NoBits>
    void verify_tiny_pair_impl(bool compressed)
    {
        REQUIRE(tiny_pair_impl<spare_test_type, NoBits>::is_compressed::value == compressed);

        tiny_pair_impl<spare_test_type, NoBits> pair(spare_test_type{}, 0u);
        REQUIRE(pair.integer() == 0u);
        REQUIRE(pair.big().spare_bits == UINT16_MAX);

        pair.set_integer(42);
        REQUIRE(pair.integer() == 42);
        REQUIRE(pair.big().spare_bits == UINT16_MAX);

        pair.modify_big()->verify();
        REQUIRE(pair.integer() == 42);
        REQUIRE(pair.big().spare_bits == UINT16_MAX);

        if (compressed)
        {
            // this only works if it is compressed
            // otherwise the pair leaves the spare bits alone (as it should)
            pair.modify_big()->spare_bits = 43;
            REQUIRE(pair.integer() == 42);
            REQUIRE(pair.big().spare_bits == UINT16_MAX);
        }

        *pair.modify_big() = spare_test_type{};
        REQUIRE(pair.integer() == 42);
        REQUIRE(pair.big().spare_bits == UINT16_MAX);

        pair = decltype(pair)(spare_test_type{}, 17u);
        REQUIRE(pair.integer() == 17u);
        REQUIRE(pair.big().spare_bits == UINT16_MAX);

        decltype(pair) copy(pair);
        REQUIRE(copy.integer() == 17u);
        REQUIRE(copy.big().spare_bits == UINT16_MAX);
    }
} // namespace

TEST_CASE("tiny_pair_impl")
{
    SECTION("compressed")
    {
        verify_tiny_pair_impl<8u>(true);
        verify_tiny_pair_impl<16u>(true);
    }
    SECTION("not compressed")
    {
        verify_tiny_pair_impl<17u>(false);
        verify_tiny_pair_impl<32u>(false);
    }
    SECTION("no bits")
    {
        tiny_pair_impl<spare_test_type, 0u> pair({}, 0u);
        REQUIRE(pair.integer() == 0u);
        REQUIRE(pair.big().spare_bits == UINT16_MAX);

        pair.modify_big()->verify();
    }
}

namespace
{
    void verify_pair(const tiny_bool_pair<spare_test_type>& pair, bool value)
    {
        REQUIRE(pair.first().spare_bits == UINT16_MAX);
        REQUIRE(pair.second() == value);

        REQUIRE(get<0>(pair).spare_bits == UINT16_MAX);
        REQUIRE(get<1>(pair) == value);

        REQUIRE(pair == make_tiny_pair(spare_test_type{}, value));
        REQUIRE(pair != make_tiny_pair(spare_test_type{}, !value));
        REQUIRE(pair <= make_tiny_pair(spare_test_type{}, true));
        REQUIRE(pair >= make_tiny_pair(spare_test_type{}, false));
    }
} // namespace

TEST_CASE("tiny_bool_pair")
{
    SECTION("default constructor")
    {
        tiny_bool_pair<spare_test_type> pair;
        verify_pair(pair, false);

        pair.modify_first()->spare_bits = 42;
        verify_pair(pair, false);

        pair.set_first({});
        verify_pair(pair, false);

        pair.set_second(true);
        verify_pair(pair, true);

        SECTION("assignment")
        {
            pair = tiny_bool_pair<spare_test_type>();
            verify_pair(pair, false);

            pair = std::make_pair(spare_test_type{}, true);
            verify_pair(pair, true);
        }
        SECTION("copy")
        {
            tiny_bool_pair<spare_test_type> copy(pair);
            verify_pair(copy, true);
        }
    }
    SECTION("value constructor")
    {
        tiny_bool_pair<spare_test_type> pair(spare_test_type{}, true);
        verify_pair(pair, true);
    }
    SECTION("tuple constructor")
    {
        tiny_bool_pair<spare_test_type> pair(std::make_pair(spare_test_type{}, false));
        verify_pair(pair, false);
    }
    SECTION("make_tiny_pair")
    {
        tiny_bool_pair<spare_test_type> pair = make_tiny_pair(spare_test_type{}, true);
        verify_pair(pair, true);
    }
}
