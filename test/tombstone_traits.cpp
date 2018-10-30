// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/tombstone.hpp>

#include <catch.hpp>

#include <foonathan/tiny/optional_impl.hpp>
#include <foonathan/tiny/pointer_tiny_storage.hpp>

using namespace foonathan::tiny;

namespace
{
template <typename T>
struct storage
{
    using traits = tombstone_traits<T>;
    typename traits::storage_type storage;

    template <typename... Args>
    typename traits::reference create_object(Args&&... args)
    {
        traits::create_object(storage, std::forward<Args>(args)...);
        return traits::get_object(storage);
    }

    void destroy_object()
    {
        traits::destroy_object(storage);
    }

    void create_tombstone(std::size_t index)
    {
        traits::create_tombstone(storage, index);
    }

    std::size_t tombstone_count()
    {
        return traits::tombstone_count;
    }

    std::size_t tombstone()
    {
        return traits::get_tombstone(storage);
    }
};

template <typename T>
void verify_object(const typename tombstone_traits<T>::object_type& obj)
{
    storage<T> s;
    auto&&     ref = s.create_object(obj);
    REQUIRE(ref == obj);
    REQUIRE(s.tombstone() >= s.tombstone_count());
}

template <typename T>
void verify_object(const T& obj)
{
    verify_object<T>(obj);
}

template <typename T>
void verify_tombstones(std::size_t tc)
{
    storage<T> s;
    REQUIRE(s.tombstone_count() == tc);

    for (auto i = 0u; i != s.tombstone_count(); ++i)
    {
        s.create_tombstone(i);
        REQUIRE(s.tombstone() == i);
    }
}
} // namespace

TEST_CASE("tombstone_traits default")
{
    verify_tombstones<std::string>(0);
    verify_object(std::string("Hello World!"));
    verify_object(std::string());
}

namespace
{
struct padded_and_layout
{
    std::uint8_t  a;
    std::uint16_t b;

    friend bool operator==(padded_and_layout lhs, padded_and_layout rhs)
    {
        return lhs.a == rhs.a && lhs.b == rhs.b;
    }
};

struct only_padded
{
    std::uint8_t  a;
    std::uint16_t b;

    // no default ctor
    explicit only_padded(std::uint8_t a, std::uint16_t b) : a(a), b(b) {}

    // not trivial
    only_padded(const only_padded& other) : a(other.a), b(other.b) {}

    friend bool operator==(only_padded lhs, only_padded rhs)
    {
        return lhs.a == rhs.a && lhs.b == rhs.b;
    }
};
} // namespace

namespace foonathan
{
namespace tiny
{
    template <>
    struct padding_traits<padded_and_layout>
    : padding_traits_aggregate<FOONATHAN_TINY_MEMBER(padded_and_layout, a),
                               FOONATHAN_TINY_MEMBER(padded_and_layout, b)>
    {};

    template <>
    struct tombstone_traits<only_padded> : tombstone_traits_padded<only_padded, padded_and_layout>
    {};
} // namespace tiny
} // namespace foonathan

TEST_CASE("tombstone_traits padded")
{
    SECTION("automatic")
    {
        verify_tombstones<padded_and_layout>(255);
        verify_object(padded_and_layout{0, 0});
        verify_object(padded_and_layout{255, 1642});
    }
    SECTION("manual")
    {
        verify_tombstones<only_padded>(255);
        verify_object(only_padded{0, 0});
        verify_object(only_padded{255, 1642});
    }
}

TEST_CASE("tombstone_traits tiny")
{
    SECTION("bool")
    {
        verify_tombstones<bool>(127);
        verify_object(true);
        verify_object(false);
    }
    SECTION("tiny_int")
    {
        verify_tombstones<tiny_unsigned<4>>(15);
        for (auto i = 0u; i != 16; ++i)
            verify_object<tiny_unsigned<4>>(i);
    }
    SECTION("enum")
    {
        enum class foo
        {
            a,
            b,
            c,
            unsigned_count_,
        };

        verify_tombstones<foo>(63);
        verify_object(foo::a);
        verify_object(foo::b);
        verify_object(foo::c);
    }
}

TEST_CASE("tombstone_traits optional_impl")
{
    SECTION("not compressed")
    {
        using opt_t = optional_impl<std::string>;
        verify_tombstones<opt_t>(tombstone_traits<bool>::tombstone_count);

        storage<opt_t> s;
        auto&          ref = s.create_object();
        REQUIRE(s.tombstone() >= s.tombstone_count());
        REQUIRE(!ref.has_value());

        ref.create_value("hello world!");
        REQUIRE(s.tombstone() >= s.tombstone_count());
        REQUIRE(ref.has_value());
        REQUIRE(ref.value() == "hello world!");

        ref.destroy_value();
        REQUIRE(s.tombstone() >= s.tombstone_count());
        REQUIRE(!ref.has_value());
    }
    SECTION("compressed")
    {
        using opt_t = optional_impl<bool>;
        verify_tombstones<opt_t>(tombstone_traits<bool>::tombstone_count - 1);

        storage<opt_t> s;
        auto&          ref = s.create_object();
        REQUIRE(s.tombstone() >= s.tombstone_count());
        REQUIRE(!ref.has_value());

        ref.create_value(true);
        REQUIRE(s.tombstone() >= s.tombstone_count());
        REQUIRE(ref.has_value());
        REQUIRE(ref.value() == true);

        ref.destroy_value();
        REQUIRE(s.tombstone() >= s.tombstone_count());
        REQUIRE(!ref.has_value());
    }
}

TEST_CASE("tombstone_traits pointer")
{
    SECTION("alignment == 1")
    {
        verify_tombstones<const char*>(0);
        verify_object<const char*>("hello world!");
        verify_object<const char*>(nullptr);
    }
    SECTION("alignment == 2")
    {
        verify_tombstones<std::uint16_t*>(1);

        std::uint16_t obj;
        verify_object(&obj);
        verify_object(static_cast<std::uint16_t*>(nullptr));
    }
    SECTION("alignment == 4")
    {
        verify_tombstones<std::uint32_t*>(3);

        std::uint32_t obj;
        verify_object(&obj);
        verify_object(static_cast<std::uint32_t*>(nullptr));
    }
    SECTION("overriden alignment")
    {
        verify_tombstones<aligned_obj<char, 8>*>(7);

        alignas(8) char obj;
        verify_object<aligned_obj<char, 8>*>(&obj);
        verify_object<aligned_obj<char, 8>*>(nullptr);
    }
}
