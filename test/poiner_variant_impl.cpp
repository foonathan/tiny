// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/pointer_variant_impl.hpp>

#include <catch.hpp>

using namespace foonathan::tiny;

namespace
{
    template <typename T>
    struct get_pointer
    {
        using type = T;

        static T* get()
        {
            return reinterpret_cast<T*>(std::uintptr_t(1024));
        }
    };

    template <typename T, std::size_t Alignment>
    struct get_pointer<aligned_obj<T, Alignment>>
    {
        using type = T;

        static T* get()
        {
            return reinterpret_cast<T*>(std::uintptr_t(1024));
        }
    };

    template <typename A, typename B, typename C>
    void verify_variant_impl(bool is_compressed)
    {
        using variant = pointer_variant_impl<A, B, C>;
        REQUIRE(typename variant::is_compressed{} == is_compressed);

        auto a_ptr    = get_pointer<A>::get();
        using a_type  = typename get_pointer<A>::type;
        using a_tag_t = typename variant::template tag_of<A>;
        REQUIRE(typename a_tag_t::is_valid{});
        auto a_tag = a_tag_t::value;

        auto b_ptr    = get_pointer<B>::get();
        using b_type  = typename get_pointer<B>::type;
        using b_tag_t = typename variant::template tag_of<B>;
        REQUIRE(typename b_tag_t::is_valid{});
        auto b_tag = b_tag_t::value;

        auto c_ptr    = get_pointer<C>::get();
        using c_type  = typename get_pointer<C>::type;
        using c_tag_t = typename variant::template tag_of<C>;
        REQUIRE(typename c_tag_t::is_valid{});
        auto c_tag = c_tag_t::value;

        REQUIRE(a_tag != b_tag);
        REQUIRE(a_tag != c_tag);
        REQUIRE(b_tag != c_tag);

        auto verify_null = [&](const variant& v) {
            REQUIRE(!v.has_value());
            REQUIRE(v.tag() != a_tag);
            REQUIRE(v.tag() != b_tag);
            REQUIRE(v.tag() != c_tag);
            REQUIRE(v.get() == nullptr);
        };

        variant v(nullptr);
        verify_null(v);

        v.reset(a_ptr);
        REQUIRE(v.has_value());
        REQUIRE(v.tag() == a_tag);
        REQUIRE(v.get() == a_ptr);
        REQUIRE(v.template pointer_to<a_type>() == a_ptr);

        v.reset(b_ptr);
        REQUIRE(v.has_value());
        REQUIRE(v.tag() == b_tag);
        REQUIRE(v.get() == b_ptr);
        REQUIRE(v.template pointer_to<b_type>() == b_ptr);

        v.reset(nullptr);
        verify_null(v);

        v.reset(c_ptr);
        REQUIRE(v.has_value());
        REQUIRE(v.tag() == c_tag);
        REQUIRE(v.get() == c_ptr);
        REQUIRE(v.template pointer_to<c_type>() == c_ptr);

        v.reset(static_cast<a_type*>(nullptr));
        verify_null(v);
    }
} // namespace

TEST_CASE("pointer_variant_impl")
{
    SECTION("not compressed: normal")
    {
        verify_variant_impl<std::int32_t, std::int64_t, const char>(false);
    }
    SECTION("not compressed: aligned_obj")
    {
        verify_variant_impl<std::int32_t, aligned_obj<char, 2>, const char>(false);
    }
    SECTION("compressed: normal")
    {
        verify_variant_impl<std::int32_t, std::int64_t, const std::uint32_t>(true);
    }
    SECTION("compressed: aligned_obj")
    {
        verify_variant_impl<std::int32_t, aligned_obj<char, 4>, aligned_obj<signed char, 8>>(true);
    }
}
