// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/tombstone.hpp>

#include <catch.hpp>

#include <foonathan/tiny/optional_impl.hpp>

using namespace foonathan::tiny;

namespace
{
    template <typename T>
    void verify_object(const T& obj)
    {
        REQUIRE(!is_tombstone<T>(&obj));
        REQUIRE(tombstone_index<T>(&obj) >= tombstone_count<T>());
    }

    template <typename T>
    void verify_tombstones(std::size_t tc)
    {
        REQUIRE(tombstone_count<T>() == tc);

        using memory_t = typename std::aligned_storage<sizeof(T)>::type;
        memory_t mem;
        for (auto i = 0u; i != tombstone_count<T>(); ++i)
        {
            create_tombstone<T>(&mem, i);
            REQUIRE(is_tombstone<T>(&mem));
            REQUIRE(tombstone_index<T>(&mem) == i);
        }
    }
} // namespace

TEST_CASE("tombstone_traits default")
{
    verify_tombstones<std::string>(0);
    verify_object(std::string("Hello World!"));
    verify_object(std::string());

    verify_tombstones<const char*>(0);
    verify_object<const char*>("hello world!");
    verify_object<const char*>(nullptr);
}

TEST_CASE("tombstone_traits spare bits")
{
    SECTION("bool")
    {
        verify_tombstones<bool>(127);
        verify_object(true);
        verify_object(false);
    }
    SECTION("pointer")
    {
        verify_tombstones<std::uint16_t*>(1);

        std::uint16_t obj;
        verify_object(&obj);
        verify_object(static_cast<std::uint16_t*>(nullptr));
    }
}

TEST_CASE("tombstone_traits optional_impl")
{
    SECTION("not compressed")
    {
        verify_tombstones<optional_impl<std::string>>(tombstone_count<bool>());

        optional_impl<std::string> opt;
        verify_object(opt);

        opt.create_value("hello world!");
        verify_object(opt);

        opt.destroy_value();
    }
    SECTION("compressed")
    {
        verify_tombstones<optional_impl<bool>>(tombstone_count<bool>() - 1);

        optional_impl<bool> opt;
        verify_object(opt);

        opt.create_value(true);
        verify_object(opt);

        opt.destroy_value();
        opt.create_value(false);
        verify_object(opt);

        opt.destroy_value();
    }
}
