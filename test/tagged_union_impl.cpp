// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/tagged_union_impl.hpp>

#include <catch.hpp>

using namespace foonathan::tiny;

namespace
{
using types = union_types<struct A, struct B, struct C>;

struct A
{
    tagged_union_tag<types> tag;
    int                     i;

    A() : i(42) {}

    void verify() const
    {
        REQUIRE(i == 42);
    }
};

struct B
{
    tagged_union_tag<types> tag;
    using tiny_ts = tiny_types<tiny_int_range<0, 20>>;

    B()
    {
        tag.tiny_view(tiny_ts{}).tiny() = 11;
    }

    void verify() const
    {
        REQUIRE(tag.tiny_view(tiny_ts{}).tiny() == 11);
    }
};

struct C
{
    tagged_union_tag<types> tag;
    char                    additional[3];

    using tiny_ts = tiny_types<tiny_unsigned<30, unsigned>>;

    C()
    {
        tag.tiny_view(tiny_ts{}, make_bit_view<0, last_bit>(additional)).tiny() = 1u << 20;
    }

    void verify() const
    {
        REQUIRE((tag.tiny_view(tiny_ts{}, make_bit_view<0, last_bit>(additional))).tiny()
                == 1u << 20);
    }
};

template <typename T, class Union>
void verify_union(Union& u, std::size_t index)
{
    REQUIRE(u.template has_value<T>());
    REQUIRE(u.tag() == index);
    u.template value<T>().verify();
}
} // namespace

TEST_CASE("tagged_union_impl")
{
    tagged_union_impl<types> u;
    const auto&              cu = u;

    u.create_value<A>();
    verify_union<A>(u, 0);
    verify_union<A>(cu, 0);
    u.destroy_value<A>();

    u.create_value<B>();
    verify_union<B>(u, 1);
    verify_union<B>(cu, 1);
    u.destroy_value<B>();

    u.create_value<C>();
    verify_union<C>(u, 2);
    verify_union<C>(cu, 2);
    u.destroy_value<C>();
}
