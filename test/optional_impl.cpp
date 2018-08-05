// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/optional_impl.hpp>

#include <catch.hpp>

using namespace foonathan::tiny;

namespace
{
    template <typename T>
    void verify_optional_impl(const T& obj, bool is_compressed)
    {
        REQUIRE(optional_impl<T>::is_compressed::value == is_compressed);

        optional_impl<T> opt;
        REQUIRE(!opt.has_value());

        opt.create_value(obj);
        REQUIRE(opt.has_value());
        REQUIRE(opt.value() == obj);

        opt.destroy_value();
        REQUIRE(!opt.has_value());
    }
} // namespace

TEST_CASE("optional_impl")
{
    SECTION("not compressed")
    {
        verify_optional_impl(std::string("hello"), false);
        verify_optional_impl(std::string(""), false);
    }
    SECTION("compressed: bool")
    {
        verify_optional_impl(true, true);
        verify_optional_impl(false, true);
    }
    SECTION("compressed: pointer")
    {
        verify_optional_impl(static_cast<std::uint64_t*>(nullptr), true);
    }
}
