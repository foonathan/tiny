// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TINY_TYPE_HPP_INCLUDED
#define FOONATHAN_TINY_TINY_TYPE_HPP_INCLUDED

#include <foonathan/tiny/bit_view.hpp>
#include <foonathan/tiny/detail/assert.hpp>
#include <foonathan/tiny/enum_traits.hpp>

namespace foonathan
{
namespace tiny
{
#if 0
    /// A type that only occupies a couple of bits.
    struct TinyType
    {
        /// The proper object type whose size is measured in whole bytes.
        using object_type = ...;

        /// \returns The size of the object in bits.
        static constexpr std::size_t bit_size() noexcept;

        /// A proxy for `object_type`.
        ///
        /// `BitView` is a [tiny::bit_view]() viewing the `bit_size()` bits of storage the object occupies.
        /// It should implement the required functions to be a stand-in for `object_type`,
        /// but at the very least it should provide a conversion operator that converts the view into the object type
        /// and an assignment operator that changes the bits to store a new object.
        template <class BitView>
        class proxy
        {
        public:
            ...

        private:
            /// \effects Constructs it from the given `BitView`.
            explicit proxy(BitView view);

            /// The constructor is private, so you must friend `tiny_type_access`.
            friend tiny_type_access;
        };
    };
#endif

    /// This class must be friend of a `TinyType`.
    class tiny_type_access
    {
        template <class TinyType, class View>
        static auto make(View view) noexcept -> typename TinyType::template proxy<View>
        {
            return typename TinyType::template proxy<View>(view);
        }

        template <class TinyType, typename Integer, std::size_t Begin, std::size_t End>
        friend auto make_tiny_proxy(bit_view<Integer, Begin, End> view) noexcept ->
            typename TinyType::template proxy<bit_view<Integer, Begin, End>>;
    };

    /// Creates a `TinyType` proxy viewing the given bits.
    template <class TinyType, typename Integer, std::size_t Begin, std::size_t End>
    auto make_tiny_proxy(bit_view<Integer, Begin, End> view) noexcept ->
        typename TinyType::template proxy<bit_view<Integer, Begin, End>>
    {
        static_assert(view.size() == TinyType::bit_size(), "invalid size");
        return tiny_type_access::make<TinyType>(view);
    }

    //=== tiny types ===//
    /// A `TinyType` implementation of `bool`.
    class tiny_bool
    {
    public:
        using object_type = bool;

        static constexpr std::size_t bit_size() noexcept
        {
            return 1u;
        }

        template <class BitView>
        class proxy
        {
        public:
            proxy& operator=(bool value) noexcept
            {
                view_.put(value ? 1 : 0);
                return *this;
            }

            explicit operator bool() const noexcept
            {
                return view_.extract() != 0;
            }

            friend bool operator==(proxy lhs, proxy rhs) noexcept
            {
                return !lhs == !rhs;
            }
            friend bool operator==(proxy lhs, bool rhs) noexcept
            {
                return !lhs == !rhs;
            }
            friend bool operator==(bool lhs, proxy rhs) noexcept
            {
                return !lhs == !rhs;
            }

            friend bool operator!=(proxy lhs, proxy rhs) noexcept
            {
                return !(lhs == rhs);
            }
            friend bool operator!=(proxy lhs, bool rhs) noexcept
            {
                return !(lhs == rhs);
            }
            friend bool operator!=(bool lhs, proxy rhs) noexcept
            {
                return !(lhs == rhs);
            }

        private:
            explicit proxy(BitView view) noexcept : view_(view) {}

            BitView view_;

            friend tiny_type_access;
        };
    };

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

    /// A `TinyType` implementation of a small unsigned integer type.
    ///
    /// The integer type has the specified size in bits and uses the specified type as the object
    /// type. Overflow is checked in debug mode whenever the integer is stored.
    ///
    /// \require `Integer` must be an unsigned integer type with at least `Bits` bits.
    template <std::size_t Bits, typename Integer = unsigned>
    class tiny_unsigned
    {
        static_assert(std::is_integral<Integer>::value && std::is_unsigned<Integer>::value,
                      "integer must be unsigned");
        static_assert(Bits <= sizeof(Integer) * CHAR_BIT, "integer type overflow");

    public:
        using object_type = Integer;

        static constexpr std::size_t bit_size() noexcept
        {
            return Bits;
        }

        template <class BitView>
        class proxy
        {
        public:
            operator object_type() const noexcept
            {
                return get();
            }

            const proxy& operator=(object_type value) const noexcept
            {
                DEBUG_ASSERT((are_only_bits<0, bit_size()>(value)), detail::precondition_handler{},
                             "overflow in tiny unsigned");
                view_.put(value);
                return *this;
            }

            const proxy& operator+=(object_type i) const noexcept
            {
                return *this = static_cast<object_type>(get() + i);
            }
            const proxy& operator-=(object_type i) const noexcept
            {
                return *this = static_cast<object_type>(get() - i);
            }
            const proxy& operator*=(object_type i) const noexcept
            {
                return *this = static_cast<object_type>(get() * i);
            }
            const proxy& operator/=(object_type i) const noexcept
            {
                return *this = static_cast<object_type>(get() / i);
            }
            const proxy& operator%=(object_type i) const noexcept
            {
                return *this = static_cast<object_type>(get() % i);
            }

            const proxy& operator++() const noexcept
            {
                return *this += 1;
            }
            object_type operator++(int) const noexcept
            {
                auto copy = get();
                ++*this;
                return copy;
            }
            const proxy& operator--() const noexcept
            {
                return *this -= 1;
            }
            object_type operator--(int) const noexcept
            {
                auto copy = get();
                --*this;
                return copy;
            }

        private:
            explicit proxy(BitView view) noexcept : view_(view) {}

            object_type get() const noexcept
            {
                return static_cast<object_type>(view_.extract());
            }

            BitView view_;

            friend tiny_type_access;
        };
    };

    /// A `TinyType` implementation of a small signed integer type.
    ///
    /// The integer type has the specified size in bits and uses the specified type as the object
    /// type. Overflow is checked in debug mode whenever the integer is stored.
    ///
    /// \require `Integer` must be an signed integer type with at least `Bits` bits.
    template <std::size_t Bits, typename Integer = int>
    class tiny_int
    {
        static_assert(std::is_integral<Integer>::value && std::is_signed<Integer>::value,
                      "integer must be signed");
        static_assert(Bits <= sizeof(Integer) * CHAR_BIT, "integer type overflow");

    public:
        using object_type = Integer;

        static constexpr std::size_t bit_size() noexcept
        {
            return Bits;
        }

        template <class BitView>
        class proxy
        {
        public:
            operator object_type() const noexcept
            {
                return get();
            }

            const proxy& operator=(object_type value) const noexcept
            {
                assign(value);
                return *this;
            }

            const proxy& operator+=(object_type i) const noexcept
            {
                return *this = static_cast<object_type>(get() + i);
            }
            const proxy& operator-=(object_type i) const noexcept
            {
                return *this = static_cast<object_type>(get() - i);
            }
            const proxy& operator*=(object_type i) const noexcept
            {
                return *this = static_cast<object_type>(get() * i);
            }
            const proxy& operator/=(object_type i) const noexcept
            {
                return *this = static_cast<object_type>(get() / i);
            }
            const proxy& operator%=(object_type i) const noexcept
            {
                return *this = static_cast<object_type>(get() % i);
            }

            const proxy& operator++() const noexcept
            {
                return *this += 1u;
            }
            object_type operator++(int) const noexcept
            {
                auto copy = get();
                ++*this;
                return copy;
            }
            const proxy& operator--() const noexcept
            {
                return *this -= 1u;
            }
            object_type operator--(int) const noexcept
            {
                auto copy = get();
                --*this;
                return copy;
            }

        private:
            explicit proxy(BitView view) noexcept : view_(view) {}

            // note: assuming two's complement representation here
            using unsigned_type = typename std::make_unsigned<Integer>::type;

            object_type get() const noexcept
            {
                // look at the most significant bit, which determines the sign
                if (!view_[Bits - 1])
                    // bit not set, positive value
                    // so just return that
                    return static_cast<object_type>(view_.extract());
                else
                {
                    // bit set, negative value
                    auto rep = static_cast<unsigned_type>(view_.extract());

                    // take complement and add one to get the absolute value
                    // and clear any overflow bits
                    auto absolute_value = static_cast<unsigned_type>(~rep + 1);
                    // clear any overflow bits
                    clear_other_bits<0, Bits>(absolute_value);
                    // return absolute value properly negated
                    return static_cast<object_type>(-static_cast<object_type>(absolute_value));
                }
            }

            static constexpr auto min = object_type(-(1ll << (Bits - 1)));
            static constexpr auto max = object_type((1ll << (Bits - 1)) - 1);

            void assign(object_type value) const noexcept
            {
                // can't do an overflow check by looking at the bits,
                // as negative values have ones in the higher bits
                DEBUG_ASSERT(min <= value, detail::precondition_handler{}, "overflow in tiny int");
                DEBUG_ASSERT(value <= max, detail::precondition_handler{}, "overflow in tiny int");

                // we can however, simply store the `Bits` lower bits,
                // as truncation will preserve signed-ness
                auto rep = reinterpret_cast<unsigned_type&>(value);
                view_.put(static_cast<std::uintmax_t>(rep));
            }

            BitView view_;

            friend tiny_type_access;
        };
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TINY_TYPE_HPP_INCLUDED
