// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TINY_INT_HPP_INCLUDED
#define FOONATHAN_TINY_TINY_INT_HPP_INCLUDED

#include <limits>

#include <foonathan/tiny/detail/ilog2.hpp>
#include <foonathan/tiny/detail/select_integer.hpp>
#include <foonathan/tiny/tiny_type.hpp>

namespace foonathan
{
namespace tiny
{
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
                    absolute_value = clear_other_bits<0, Bits>(absolute_value);
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

    /// \exclude
    namespace tiny_int_detail
    {
        template <std::intmax_t Min, std::intmax_t Max>
        constexpr std::size_t bits_for() noexcept
        {
            static_assert(Min <= Max, "invalid range");
            return detail::ilog2_ceil(static_cast<std::uintmax_t>(Max - Min));
        }

        template <std::intmax_t Min, std::intmax_t Max>
        using integer_type_for = typename std::conditional<Min >= 0, unsigned, int>::type;
    } // namespace tiny_int_detail

    /// An integer that can hold the values in the range `[Min, Max]`.
    ///
    /// It internally stores the offset from `Min`.
    /// This means that the all bits zero value corresponds to `Min`, and not `0` like the other
    /// integers.
    template <std::intmax_t Min, std::intmax_t Max,
              typename Integer = tiny_int_detail::integer_type_for<Min, Max>>
    class tiny_int_range
    {
        static_assert(std::is_integral<Integer>::value, "must be an integer");
        static_assert(std::numeric_limits<Integer>::min() <= Min
                          && Max <= std::numeric_limits<Integer>::max(),
                      "integer can't handle range");

    public:
        using object_type = Integer;

        static constexpr std::size_t bit_size() noexcept
        {
            return tiny_int_detail::bits_for<Min, Max>();
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
                DEBUG_ASSERT(Min <= value, detail::precondition_handler{},
                             "overflow in tiny_int_range");
                DEBUG_ASSERT(value <= Max, detail::precondition_handler{},
                             "overflow in tiny_int_range");
                auto offset = std::intmax_t(value) - Min;
                DEBUG_ASSERT(offset >= 0, detail::assert_handler{});
                view_.put(static_cast<std::uintmax_t>(offset));
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
                return static_cast<object_type>(std::intmax_t(view_.extract()) + Min);
            }

            BitView view_;

            friend tiny_type_access;
        };
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TINY_INT_HPP_INCLUDED
