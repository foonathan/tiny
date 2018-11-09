// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_ENUM_TRAITS_HPP_INCLUDED
#define FOONATHAN_TINY_ENUM_TRAITS_HPP_INCLUDED

#include <limits>
#include <type_traits>

#include <foonathan/tiny/detail/ilog2.hpp>

namespace foonathan
{
namespace tiny
{
    //=== enum traits ===//
    /// The default enum traits implementation.
    template <typename Enum>
    class enum_traits_default
    {
        static_assert(std::is_enum<Enum>::value, "not an enum");
        using underlying_type = typename std::underlying_type<Enum>::type;

    public:
        /// The type of the enum.
        using enum_type = Enum;

        /// Whether or not the traits are specialized for the enum.
        ///
        /// If this is `false`, no real information is available.
        /// Set to `true` in all specializations.
        static constexpr auto is_specialized = false;

        /// The first (numerically minimal) valid enum value.
        static constexpr Enum min() noexcept
        {
            return Enum(std::numeric_limits<underlying_type>::min());
        }

        /// The last (numerically maximal) valid enum value.
        static constexpr Enum max() noexcept
        {
            return Enum(std::numeric_limits<underlying_type>::max());
        }

        /// Whether or not all enum values are contiguously.
        static constexpr auto is_contiguous = false;
    };

    /// Enum traits implementation that assumes the enum values are contiguous in the range `[0,
    /// MaxValue]`.
    template <typename Enum, Enum MaxValue>
    struct enum_traits_unsigned
    {
        static_assert(std::is_enum<Enum>::value, "not an enum");

        using enum_type = Enum;

        static constexpr auto is_specialized = true;

        static constexpr Enum min() noexcept
        {
            return Enum(0);
        }

        static constexpr Enum max() noexcept
        {
            return MaxValue;
        }

        static constexpr auto is_contiguous = true;
    };

    /// Enum traits implementation that assumes the enum values are contiguous in the range `[0,
    /// Count]`.
    template <typename Enum, Enum Count>
    struct enum_traits_unsigned_count
    : enum_traits_unsigned<Enum,
                           Enum(static_cast<typename std::underlying_type<Enum>::type>(Count) - 1)>
    {};

    /// Enum traits implementation that assumes the enum values are contiguous in the range
    /// `[MinValue, MaxValue]`.
    template <typename Enum, Enum MinValue, Enum MaxValue>
    struct enum_traits_signed
    {
        static_assert(std::is_enum<Enum>::value, "not an enum");

        using enum_type = Enum;

        static constexpr auto is_specialized = true;

        static constexpr Enum min() noexcept
        {
            return MinValue;
        }
        static constexpr Enum max() noexcept
        {
            return MaxValue;
        }

        static constexpr auto is_contiguous = true;
    };

    namespace enum_traits_detail
    {
        template <std::size_t>
        struct tag
        {};

        struct overload
        {
            template <std::size_t I>
            operator tag<I>() const noexcept
            {
                return {};
            }
        };

        template <typename Enum>
        enum_traits_default<Enum> enum_traits_for(...);

        template <typename Enum>
        enum_traits_unsigned_count<Enum, Enum::_unsigned_count> enum_traits_for(tag<0>);
        template <typename Enum>
        enum_traits_unsigned_count<Enum, Enum::unsigned_count_> enum_traits_for(tag<1>);

        template <typename Enum>
        enum_traits_unsigned<Enum, Enum::_unsigned_max> enum_traits_for(tag<2>);
        template <typename Enum>
        enum_traits_unsigned<Enum, Enum::unsigned_max_> enum_traits_for(tag<3>);

        template <typename Enum>
        enum_traits_unsigned_count<Enum, Enum::_flag_count> enum_traits_for(tag<0>);
        template <typename Enum>
        enum_traits_unsigned_count<Enum, Enum::flag_count_> enum_traits_for(tag<1>);
    } // namespace enum_traits_detail

    /// The traits of an enum.
    ///
    /// Specialize it for your own types,
    /// either by manually specifying the required members or by inheriting from the provided
    /// implementations.
    ///
    /// Enums with an enumerator `_unsigned_count` or `unsigned_count_` will be treated as unsigned
    /// enums with the specified count. Enums with an enumerator `_unsigned_max` or `unsigned_max_`
    /// will be treated as unsigned enums with the specified max value. Enums with an enumerator
    /// `_flag_count` or `flag_count_` will be treated as unsigned enums with the specified count.
    /// They are meant for [tiny::flag_set]().
    template <typename Enum>
    struct enum_traits
    : decltype(enum_traits_detail::enum_traits_for<Enum>(enum_traits_detail::overload{}))
    {};

    //=== enum traits algorithm ===//
    /// \exclude
    namespace enum_traits_detail
    {
        template <bool IsEnum, class EnumOrTraits>
        struct traits_of_impl;

        template <class Enum>
        struct traits_of_impl<true, Enum>
        {
            using type = enum_traits<Enum>;
        };

        template <class Traits>
        struct traits_of_impl<false, Traits>
        {
            using type = Traits;
        };
    } // namespace enum_traits_detail

    /// If `EnumOrTraits` is an enum type, equivalent to `enum_traits<EnumOrTraits>`.
    /// Otherwise, expands to `EnumOrTraits` and assumes it has the same members as the
    /// `enum_traits`.
    template <class EnumOrTraits>
    using traits_of_enum =
        typename enum_traits_detail::traits_of_impl<std::is_enum<EnumOrTraits>::value,
                                                    EnumOrTraits>::type;

    /// \returns The size of an enum, that is the number of valid enum values.
    /// \requires The enum must be contiguous.
    template <class EnumOrTraits>
    constexpr std::size_t enum_size() noexcept
    {
        using traits = traits_of_enum<EnumOrTraits>;
        static_assert(traits::is_contiguous, "enum must be contiguous");
        return std::size_t(traits::max()) - std::size_t(traits::min()) + 1;
    }

    /// \returns The number of bits required to store a valid enum value.
    /// \requires The enum must be contiguous.
    template <class EnumOrTraits>
    constexpr std::size_t enum_bit_size() noexcept
    {
        return detail::ilog2_ceil(enum_size<EnumOrTraits>());
    }

    /// \returns Whether or not the given enum value is a valid value.
    /// \requires The enum must be contiguous.
    /// \group is_valid_enum_value
    template <typename Enum>
    constexpr bool is_valid_enum_value(Enum value) noexcept
    {
        return is_valid_enum_value<enum_traits<Enum>>(value);
    }
    /// \group is_valid_enum_value
    template <class Traits, typename Enum>
    constexpr bool is_valid_enum_value(Enum value) noexcept
    {
        static_assert(Traits::is_contiguous, "enum must be contiguous");
        using underlying = typename std::underlying_type<Enum>::type;
        return underlying(Traits::min()) <= underlying(value)
               && underlying(value) <= underlying(Traits::max());
    }
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_ENUM_TRAITS_HPP_INCLUDED
