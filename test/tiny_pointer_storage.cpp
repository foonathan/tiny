// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/tiny_pointer_storage.hpp>

#include <catch.hpp>

#include <foonathan/tiny/tiny_int.hpp>

using namespace foonathan::tiny;

namespace
{
template <class Storage>
void verify_pointer_assignment(Storage& s, unsigned tiny_value)
{
    alignas(16) typename Storage::value_type array[4];

    s.pointer() = array;
    REQUIRE(s.tiny() == tiny_value);
    REQUIRE(s.pointer() == array);

    s.pointer() += 2;
    REQUIRE(s.tiny() == tiny_value);
    REQUIRE(s.pointer() == array + 2);

    s.pointer() -= 1;
    REQUIRE(s.tiny() == tiny_value);
    REQUIRE(s.pointer() == array + 1);

    auto ptr = s.pointer()++;
    REQUIRE(s.tiny() == tiny_value);
    REQUIRE(s.pointer() == array + 2);
    REQUIRE(ptr == array + 1);

    ptr = ++s.pointer();
    REQUIRE(s.tiny() == tiny_value);
    REQUIRE(s.pointer() == array + 3);
    REQUIRE(ptr == array + 3);

    ptr = s.pointer()--;
    REQUIRE(s.tiny() == tiny_value);
    REQUIRE(s.pointer() == array + 2);
    REQUIRE(ptr == array + 3);

    ptr = --s.pointer();
    REQUIRE(s.tiny() == tiny_value);
    REQUIRE(s.pointer() == array + 1);
    REQUIRE(ptr == array + 1);
}
} // namespace

TEST_CASE("tiny_pointer_storage")
{
    SECTION("compressed")
    {
        using storage = tiny_pointer_storage<std::uint32_t, tiny_unsigned<2>>;
        REQUIRE(storage::is_compressed::value);

        storage s;
        REQUIRE(s.tiny() == 0);
        REQUIRE(s.pointer() == nullptr);

        s.tiny() = 3;
        REQUIRE(s.tiny() == 3);
        REQUIRE(s.pointer() == nullptr);

        const auto& cs = s;
        REQUIRE(cs.tiny() == 3);
        REQUIRE(cs.pointer() == nullptr);

        verify_pointer_assignment(s, 3);
    }
    SECTION("compressed custom alignment")
    {
        using storage = tiny_pointer_storage<aligned_obj<int, 8>, tiny_unsigned<3>>;
        REQUIRE(storage::is_compressed::value);

        storage s;
        REQUIRE(s.tiny() == 0);
        REQUIRE(s.pointer() == nullptr);

        s.tiny() = 7;
        REQUIRE(s.tiny() == 7);
        REQUIRE(s.pointer() == nullptr);

        const auto& cs = s;
        REQUIRE(cs.tiny() == 7);
        REQUIRE(cs.pointer() == nullptr);

        alignas(8) int value = 0;
        s.pointer()          = &value;
        REQUIRE(s.tiny() == 7);
        REQUIRE(s.pointer() == &value);
    }
    SECTION("not compressed")
    {
        using storage = tiny_pointer_storage<std::uint32_t, tiny_unsigned<3>>;
        REQUIRE(!storage::is_compressed::value);

        storage s;
        REQUIRE(s.tiny() == 0);
        REQUIRE(s.pointer() == nullptr);

        s.tiny() = 7;
        REQUIRE(s.tiny() == 7);
        REQUIRE(s.pointer() == nullptr);

        const auto& cs = s;
        REQUIRE(cs.tiny() == 7);
        REQUIRE(cs.pointer() == nullptr);

        verify_pointer_assignment(s, 7);
    }
    SECTION("big not compressed")
    {
        using storage = tiny_pointer_storage<
            std::uint32_t, tiny_unsigned<2 + sizeof(std::uintptr_t) * CHAR_BIT / 2, std::uint64_t>,
            tiny_unsigned<sizeof(std::uintptr_t) * CHAR_BIT / 2, std::uint64_t>>;
        REQUIRE(!storage::is_compressed::value);
        REQUIRE(sizeof(storage) == 2 * sizeof(std::uintptr_t));
    }
}
