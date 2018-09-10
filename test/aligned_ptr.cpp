// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/aligned_ptr.hpp>

#include <catch.hpp>

using namespace foonathan::tiny;

namespace
{
    using test_ptr = aligned_ptr<aligned_obj<int, 8>>;
    static_assert(std::is_same<test_ptr, aligned_ptr<int, 8>>::value, "");

    void verify_ptr(test_ptr test, int* expected)
    {
        if (expected)
        {
            REQUIRE(test);
            REQUIRE(*test == *expected);
            REQUIRE(test.operator->() == expected);

            REQUIRE_FALSE(test == nullptr);
            REQUIRE_FALSE(nullptr == test);
            REQUIRE(test != nullptr);
            REQUIRE(nullptr != test);
        }
        else
        {
            REQUIRE_FALSE(test);

            REQUIRE(test == nullptr);
            REQUIRE(nullptr == test);
            REQUIRE_FALSE(test != nullptr);
            REQUIRE_FALSE(nullptr != test);
        }

        REQUIRE(test.get() == expected);

        REQUIRE(test == expected);
        REQUIRE(expected == test);
        REQUIRE(test == aligned_ptr<int, 4u>(expected));

        REQUIRE_FALSE(test != expected);
        REQUIRE_FALSE(expected != test);
        REQUIRE_FALSE(test != aligned_ptr<int, 4u>(expected));
    }
} // namespace

TEST_CASE("aligned_ptr")
{
    test_ptr ptr;
    verify_ptr(ptr, nullptr);

    alignas(8) int obj;
    ptr = test_ptr(&obj);
    verify_ptr(ptr, &obj);
}

TEST_CASE("alignment_of")
{
    REQUIRE(alignment_of<int>() == alignof(int));
    REQUIRE(alignment_of<aligned_obj<int, 64>>() == 64);
}
