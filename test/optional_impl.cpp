// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/optional_impl.hpp>

#include <catch.hpp>

#include <foonathan/tiny/tiny_enum.hpp>

using namespace foonathan::tiny;

namespace
{
template <typename T>
void verify_optional_impl(const T& obj, bool is_compressed)
{
#if !defined(_MSC_VER)
    // types with deleted ctors are not trivial on MSVC
    REQUIRE(std::is_trivially_copyable<T>::value
            == std::is_trivially_copyable<optional_impl<T>>::value);
#endif
    REQUIRE(optional_impl<T>::is_compressed::value == is_compressed);

    optional_impl<T> opt;
    REQUIRE(!opt.has_value());

    opt.create_value(obj);
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == obj);

    opt.destroy_value();
    REQUIRE(!opt.has_value());
}

enum class foo
{
    a,
    b,
    c,
    _unsigned_count,
};
} // namespace

namespace foonathan
{
namespace tiny
{
    template <>
    struct tombstone_traits<foo> : tombstone_traits<tiny_enum<foo>>
    {};
} // namespace tiny
} // namespace foonathan

TEST_CASE("optional_impl")
{
    SECTION("not compressed")
    {
        verify_optional_impl(std::string("hello"), false);
        verify_optional_impl(std::string(""), false);
    }
    SECTION("not compressed trivial")
    {
        verify_optional_impl(0, false);
        verify_optional_impl(42, false);
        verify_optional_impl(INT_MAX, false);
    }
    SECTION("compressed: bool")
    {
        verify_optional_impl(true, true);
        verify_optional_impl(false, true);
    }
    SECTION("compressed: enum")
    {
        verify_optional_impl(foo::a, true);
        verify_optional_impl(foo::b, true);
        verify_optional_impl(foo::c, true);
    }
    SECTION("compressed: optional optional bool")
    {
        using opt_t = optional_impl<optional_impl<bool>>;

#if !defined(_MSC_VER)
        // types with deleted ctors are not trivial on MSVC
        REQUIRE(std::is_trivially_copyable<opt_t>::value);
#endif
        REQUIRE(opt_t::is_compressed::value);

        opt_t opt;
        REQUIRE(!opt.has_value());

        opt.create_value();
        REQUIRE(opt.has_value());
        REQUIRE(!opt.value().has_value());

        opt.value().create_value(true);
        REQUIRE(opt.has_value());
        REQUIRE(opt.value().has_value());
        REQUIRE(opt.value().value() == true);

        opt.value().destroy_value();
        REQUIRE(opt.has_value());
        REQUIRE(!opt.value().has_value());

        opt.value().create_value(false);
        REQUIRE(opt.has_value());
        REQUIRE(opt.value().has_value());
        REQUIRE(opt.value().value() == false);

        opt.destroy_value();
        REQUIRE(!opt.has_value());
    }
    SECTION("compressed: optional optional int")
    {
        using opt_t = optional_impl<optional_impl<int>>;
#if !defined(_MSC_VER)
        // types with deleted ctors are not trivial on MSVC
        REQUIRE(std::is_trivially_copyable<opt_t>::value);
#endif
        REQUIRE(opt_t::is_compressed::value);

        opt_t opt;
        REQUIRE(!opt.has_value());

        opt.create_value();
        REQUIRE(opt.has_value());
        REQUIRE(!opt.value().has_value());

        opt.value().create_value(0);
        REQUIRE(opt.has_value());
        REQUIRE(opt.value().has_value());
        REQUIRE(opt.value().value() == 0);

        opt.value().destroy_value();
        REQUIRE(opt.has_value());
        REQUIRE(!opt.value().has_value());

        opt.value().create_value(42);
        REQUIRE(opt.has_value());
        REQUIRE(opt.value().has_value());
        REQUIRE(opt.value().value() == 42);

        opt.destroy_value();
        REQUIRE(!opt.has_value());
    }
}
