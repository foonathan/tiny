// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TINY_ENUM_HPP_INCLUDED
#define FOONATHAN_TINY_TINY_ENUM_HPP_INCLUDED

#include <foonathan/tiny/enum_traits.hpp>
#include <foonathan/tiny/tiny_type.hpp>

namespace foonathan
{
namespace tiny
{
    /// A `TinyType` implementation of an enumeration type.
    ///
    /// `EnumOrTraits` is either an enum type or an `enum_traits`-like type.
    /// [lex::traits_of_enum]() is then applied.
    ///
    /// \requires The enum must be contiguous with the valid enumerators stored in the range `[0,
    /// Traits::max]`.
    template <class EnumOrTraits>
    class tiny_enum
    {
        using traits = traits_of_enum<EnumOrTraits>;
        static_assert(traits::is_contiguous, "enum must be contiguous");
        static_assert(traits::min() == typename traits::enum_type(0), "enum value must start at 0");

    public:
        using object_type = typename traits::enum_type;

        static constexpr std::size_t bit_size() noexcept
        {
            return enum_bit_size<EnumOrTraits>();
        }

        template <class BitView>
        class proxy
        {
        public:
            const proxy& operator=(object_type value) const noexcept
            {
                DEBUG_ASSERT(is_valid_enum_value<traits>(value), detail::precondition_handler{},
                             "not a valid enum value");
                view_.put(static_cast<std::uintmax_t>(value));
                return *this;
            }

            operator object_type() const noexcept
            {
                return static_cast<object_type>(view_.extract());
            }

        private:
            explicit proxy(BitView view) noexcept : view_(view) {}

            BitView view_;

            friend tiny_type_access;
        };
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TINY_ENUM_HPP_INCLUDED
