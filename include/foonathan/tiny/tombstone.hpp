// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TOMBSTONE_HPP_INCLUDED
#define FOONATHAN_TINY_TOMBSTONE_HPP_INCLUDED

#include <cstddef>
#include <new>

#include <foonathan/tiny/spare_bits.hpp>

namespace foonathan
{
namespace tiny
{
    //=== tombstone traits implementations ===//
    /// The default implementation of the tombstone traits, i.e. no tombstones.
    template <typename T>
    struct tombstone_traits_default
    {
        /// Whether or not the tombstones overlap with the spare bits.
        ///
        /// If this is `true`, an object where the spare bits are used is falsely identified as a
        /// tombstone.
        using overlaps_spare_bits = std::false_type;

        /// The number of tombstones in the type.
        static constexpr std::size_t tombstone_count = 0u;

        /// Creates the tombstone with the specified index.
        /// \effects Creates an object of some type with the same size and alignment as `T` at the
        /// given memory address, where the bit pattern uniquely identifies the given tombstone.
        /// \requires `tombstone_index < tombstone_count` and `memory` points to empty memory
        /// suitable for a `T`. \notes Tombstones must be trivial types.
        static void create_tombstone(void* memory, std::size_t tombstone_index) noexcept
        {
            (void)memory;
            (void)tombstone_index;
        }

        /// Calculates the tombstone index of the object stored at the given address.
        /// \returns The index of the given tombstone, if `memory` points to an address where the
        /// bit pattern corresponds to a tombstone. A value `>= tombstone_count` otherwise.
        /// \requires `memory` points to memory where either a valid `T` object has been constructed
        /// or a tombstone.
        static std::size_t tombstone_index(const void* memory) noexcept
        {
            (void)memory;
            return std::size_t(-1);
        }
    };

    /// A tombstone traits implementation that uses the spare bits to mark the tombstones.
    /// \requires The type must be default constructible and provide spare bits.
    /// \notes If this implementation is used tombstones cannot be distinguished from objects where
    /// the spare bits are used.
    template <typename T>
    struct tombstone_traits_spare_bits
    {
        static_assert(std::is_default_constructible<T>::value,
                      "type must be default constructible");
        static_assert(spare_bits<T>() > 0u, "not enough spare bits");

        using overlaps_spare_bits = std::true_type;

        // 2^spare_bits - 1 tombstones are available (all 0 is used for the real object)
        static constexpr std::size_t tombstone_count = (1u << spare_bits<T>()) - 1u;

        static void create_tombstone(void* memory, std::size_t tombstone_index) noexcept
        {
            // create a new object
            auto ptr = ::new (memory) T();
            // and fill the spare bits
            put_spare_bits(*ptr, tombstone_index + 1u);
        }

        static std::size_t tombstone_index(const void* memory) noexcept
        {
            // precondition: memory points to tombstone or T
            // as a tombstone is a T, we can always cast to a T
            auto ptr = static_cast<const T*>(memory);
            // if extract_spare_bits() == 0: we overflow and return something > tombstone_count
            // otherwise we return the correct index
            return extract_spare_bits(*ptr) - 1u;
        }
    };

    //=== tombstone traits ===//
    /// The tombstone traits of a given type.
    ///
    /// It will use the spare bits if the type has them and is default constructible,
    /// otherwise the type has no tombstones.
    template <typename T>
    struct tombstone_traits
    : std::conditional<std::is_default_constructible<T>::value && (spare_bits<T>() > 0u),
                       tombstone_traits_spare_bits<T>, tombstone_traits_default<T>>::type
    {};

    //=== tombstone traits algorithm ===//
    /// Whether or not the tombstones overlap with the spare bits.
    ///
    /// If this is `true`, an object where the spare bits are used is falsely identified as a
    /// tombstone.
    template <typename T>
    using tombstone_overlaps_spare_bits = typename tombstone_traits<T>::overlaps_spare_bits;

    /// \returns The number of tombstones in the given type.
    template <typename T>
    constexpr std::size_t tombstone_count() noexcept
    {
        return tombstone_traits<T>::tombstone_count;
    }

    /// Creates the tombstone with the specified index.
    /// \effects Creates an object of some type with the same size and alignment as `T` at the given
    /// memory address, where the bit pattern uniquely identifies the given tombstone. \requires
    /// `tombstone_index < tombstone_count` and `memory` points to empty memory suitable for a `T`.
    template <typename T>
    void create_tombstone(void* memory, std::size_t tombstone_index) noexcept
    {
        DEBUG_ASSERT(tombstone_index < tombstone_count<T>(), detail::precondition_handler{});
        tombstone_traits<T>::create_tombstone(memory, tombstone_index);
    }

    /// Calculates the tombstone index of the object stored at the given address.
    /// \returns The index of the given tombstone, if `memory` points to an address where the bit
    /// pattern corresponds to a tombstone. A value `>= tombstone_count` otherwise. \requires
    /// `memory` points to memory where either a valid `T` object has been constructed or a
    /// tombstone.
    template <typename T>
    std::size_t tombstone_index(const void* memory) noexcept
    {
        return tombstone_traits<T>::tombstone_index(memory);
    }

    /// \returns Whether or not the object stored at the given address is a tombstone.
    template <typename T>
    bool is_tombstone(const void* memory) noexcept
    {
        return tombstone_index<T>(memory) < tombstone_count<T>();
    }
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TOMBSTONE_HPP_INCLUDED
