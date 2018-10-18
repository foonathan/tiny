// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_BIT_FIELD_HPP_INCLUDED
#define FOONATHAN_TINY_BIT_FIELD_HPP_INCLUDED

#include <cstddef>

#include <foonathan/tiny/bit_view.hpp>
#include <foonathan/tiny/detail/select_integer.hpp>
#include <foonathan/tiny/spare_bits.hpp>

namespace foonathan
{
namespace tiny
{
    namespace detail
    {
        //=== bit_field_storage ===//
        template <typename... Fields>
        struct total_bits;

        template <>
        struct total_bits<> : std::integral_constant<std::size_t, 0>
        {};

        template <typename Head, typename... Tail>
        struct total_bits<Head, Tail...>
        : std::integral_constant<std::size_t, Head::value + total_bits<Tail...>::value>
        {};

        template <typename... Fields>
        using bit_fields_storage
            = unsigned char[total_bits<Fields...>::value / CHAR_BIT
                            + (total_bits<Fields...>::value % CHAR_BIT == 0 ? 0 : 1)];

        //=== field_bit_view ===//
        template <typename Field, typename... Fields>
        struct field_begin_offset;

        template <typename Field>
        struct field_begin_offset<Field>
        {
            static_assert(sizeof(Field) != sizeof(Field), "not part of the bit fields");
        };

        template <typename Field, typename... Tail>
        struct field_begin_offset<Field, Field, Tail...> : std::integral_constant<std::size_t, 0>
        {};

        template <typename Field, typename Head, typename... Tail>
        struct field_begin_offset<Field, Head, Tail...>
        : std::integral_constant<std::size_t,
                                 Head::value + field_begin_offset<Field, Tail...>::value>
        {};

        template <typename Field, typename... Fields>
        using field_bit_view
            = bit_view<bit_fields_storage<Fields...>, field_begin_offset<Field, Fields...>::value,
                       field_begin_offset<Field, Fields...>::value + Field::value>;

        template <class Integer>
        using enable_field_integer =
            typename std::enable_if<std::is_integral<Integer>::value
                                    && !std::is_same<Integer, bool>::value
                                    && std::is_unsigned<Integer>::value>::type;

        struct bit_field_ctor_key
        {};

        template <class Field, class... Fields>
        class basic_bit_field_reference
        {
        public:
            static constexpr std::size_t bits() noexcept
            {
                return Field::value;
            }

            explicit basic_bit_field_reference(bit_field_ctor_key,
                                               bit_fields_storage<Fields...>& storage) noexcept
            : view_(storage)
            {}

        protected:
            ~basic_bit_field_reference() = default;

            template <class Integer, typename = enable_field_integer<Integer>>
            Integer value_impl() const noexcept
            {
                static_assert(sizeof(Integer) * CHAR_BIT >= Field::value,
                              "overflow in integer type");
                return static_cast<Integer>(view_.extract());
            }

            template <class Integer, typename = enable_field_integer<Integer>>
            void assign_impl(Integer i) noexcept
            {
                static_assert(sizeof(Integer) == sizeof(Integer) && !std::is_const<Field>::value,
                              "must not modify const reference");
                DEBUG_ASSERT((bit_view<Integer, bits(), sizeof(Integer) * CHAR_BIT>(i).extract()
                              == 0),
                             detail::precondition_handler{}, "overflow in bit field");
                view_.put(static_cast<std::uintmax_t>(i));
            }

            template <class Integer, typename = enable_field_integer<Integer>>
            void assign_truncating_impl(Integer i) noexcept
            {
                static_assert(sizeof(Integer) == sizeof(Integer) && !std::is_const<Field>::value,
                              "must not modify const reference");
                view_.put(static_cast<std::uintmax_t>(i));
            }

        private:
            field_bit_view<typename std::remove_cv<Field>::type, Fields...> view_;
        };
    } // namespace detail

    /// A library implementation of bit fields.
    ///
    /// Every field must be a typedef of the form `using foo = BitFieldKind<struct foo_tag, …>`.
    /// This declares a bit field named `foo` of type `BitFieldKind` with possible additional
    /// parameters. The tag ensures that the same `BitFieldKind` with the same parameters can be
    /// passed multiple times. The name `foo` is used to access that particular field.
    /// `BitFieldKind` must be one of the types declared below.
    ///
    /// The bit fields are stored in a sufficiently sized array of `unsigned char`, exactly in the
    /// order as specified.
    template <typename... Fields>
    class bit_fields
    {
    public:
        /// \effects Initializes all bit fields to `0`.
        bit_fields() noexcept : storage_{} {}

        /// \returns A (const) reference to the specified field.
        /// \notes Simply pass the default constructed field type to invoke it.
        /// \group array_op
        template <class Field>
        auto operator[](Field) noexcept -> typename Field::template reference<Fields...>
        {
            return typename Field::template reference<Fields...>(detail::bit_field_ctor_key{},
                                                                 storage_);
        }
        /// \group array_op
        template <class Field>
        auto operator[](Field) const noexcept -> typename Field::template const_reference<Fields...>
        {
            return typename Field::template const_reference<Fields...>(detail::bit_field_ctor_key{},
                                                                       storage_);
        }

        /// \returns A pointer to stored bits.
        /// The bits are stored as a sufficiently large array of `unsigned char`.
        /// \group data
        unsigned char* data() noexcept
        {
            return storage_;
        }
        /// \group data
        const unsigned char* data() const noexcept
        {
            return storage_;
        }

    private:
        // TODO: bit_view isn't quite const-correct
        mutable detail::bit_fields_storage<Fields...> storage_;
    };

    /// A bit field that stores a boolean value.
    template <typename Tag>
    struct bit_field_bool : std::integral_constant<std::size_t, 1>
    {
        template <class... Fields>
        struct reference : detail::basic_bit_field_reference<bit_field_bool<Tag>, Fields...>
        {
            using detail::basic_bit_field_reference<bit_field_bool<Tag>,
                                                    Fields...>::basic_bit_field_reference;

            /// \effects Assigns the value to the bit field.
            reference& operator=(bool value) noexcept
            {
                this->assign_truncating_impl(value ? 1u : 0u);
                return *this;
            }

            /// \returns The value of the bit field.
            bool value() const noexcept
            {
                return static_cast<bool>(this->template value_impl<unsigned>());
            }

            /// \returns `value()`.
            explicit operator bool() const noexcept
            {
                return value();
            }

            friend bool operator==(reference lhs, reference rhs) noexcept
            {
                return lhs.value() == rhs.value();
            }
            friend bool operator==(reference lhs, bool rhs) noexcept
            {
                return lhs.value() == rhs;
            }
            friend bool operator==(bool lhs, reference rhs) noexcept
            {
                return lhs == rhs.value();
            }

            friend bool operator!=(reference lhs, reference rhs) noexcept
            {
                return lhs.value() != rhs.value();
            }
            friend bool operator!=(reference lhs, bool rhs) noexcept
            {
                return lhs.value() != rhs;
            }
            friend bool operator!=(bool lhs, reference rhs) noexcept
            {
                return lhs != rhs.value();
            }
        };

        template <class... Fields>
        struct const_reference
        : detail::basic_bit_field_reference<const bit_field_bool<Tag>, Fields...>
        {
            using detail::basic_bit_field_reference<const bit_field_bool<Tag>,
                                                    Fields...>::basic_bit_field_reference;

            /// \returns The value of the bit field.
            bool value() const noexcept
            {
                return static_cast<bool>(this->template value_impl<unsigned>());
            }

            /// \returns `value()`.
            explicit operator bool() const noexcept
            {
                return value();
            }

            friend bool operator==(const_reference lhs, const_reference rhs) noexcept
            {
                return lhs.value() == rhs.value();
            }
            friend bool operator==(const_reference lhs, bool rhs) noexcept
            {
                return lhs.value() == rhs;
            }
            friend bool operator==(bool lhs, const_reference rhs) noexcept
            {
                return lhs == rhs.value();
            }

            friend bool operator!=(const_reference lhs, const_reference rhs) noexcept
            {
                return lhs.value() != rhs.value();
            }
            friend bool operator!=(const_reference lhs, bool rhs) noexcept
            {
                return lhs.value() != rhs;
            }
            friend bool operator!=(bool lhs, const_reference rhs) noexcept
            {
                return lhs != rhs.value();
            }
        };
    };

    /// A bit field that stores an enum.
    ///
    /// The enum must only use integer values in the range `[0, MaxValue]`.
    template <typename Tag, class Enum, Enum MaxValue>
    struct bit_field_enum
    : std::integral_constant<std::size_t,
                             detail::ilog2_ceil(static_cast<std::uintmax_t>(MaxValue) + 1)>
    {
        using integer_type =
            typename std::make_unsigned<typename std::underlying_type<Enum>::type>::type;

        template <class... Fields>
        struct reference
        : detail::basic_bit_field_reference<bit_field_enum<Tag, Enum, MaxValue>, Fields...>
        {
            using detail::basic_bit_field_reference<bit_field_enum<Tag, Enum, MaxValue>,
                                                    Fields...>::basic_bit_field_reference;

            /// \effects Assigns the value to the bit field.
            reference& operator=(Enum value) noexcept
            {
                this->assign_truncating_impl(static_cast<integer_type>(value));
                return *this;
            }

            /// \returns The value of the bit field.
            Enum value() const noexcept
            {
                return static_cast<Enum>(this->template value_impl<integer_type>());
            }

            /// \returns `value()`.
            operator Enum() const noexcept
            {
                return value();
            }
        };

        template <class... Fields>
        struct const_reference
        : detail::basic_bit_field_reference<const bit_field_enum<Tag, Enum, MaxValue>, Fields...>
        {
            using detail::basic_bit_field_reference<const bit_field_enum<Tag, Enum, MaxValue>,
                                                    Fields...>::basic_bit_field_reference;

            /// \returns The value of the bit field.
            Enum value() const noexcept
            {
                return static_cast<Enum>(this->template value_impl<integer_type>());
            }

            /// \returns `value()`.
            operator Enum() const noexcept
            {
                return value();
            }
        };
    };

    /// A bit field that stores an unsigned integer with the given number of bits.
    template <typename Tag, std::size_t SizeInBits>
    struct bit_field_unsigned : std::integral_constant<std::size_t, SizeInBits>
    {
        using integer_type = detail::uint_least_n_t<SizeInBits>;

        static constexpr integer_type min() noexcept
        {
            return 0;
        }

        static constexpr integer_type max() noexcept
        {
            return static_cast<integer_type>((1ull << SizeInBits) - 1);
        }

        template <class... Fields>
        struct reference
        : detail::basic_bit_field_reference<bit_field_unsigned<Tag, SizeInBits>, Fields...>
        {
            using detail::basic_bit_field_reference<bit_field_unsigned<Tag, SizeInBits>,
                                                    Fields...>::basic_bit_field_reference;

            /// \effects Assigns the given integer value to the bit field.
            /// \requires The integer value does not overflow.
            reference& operator=(integer_type i) noexcept
            {
                this->assign_impl(i);
                return *this;
            }

            /// \returns The value of the bit field.
            integer_type value() const noexcept
            {
                return this->template value_impl<integer_type>();
            }

            /// \returns `value()`.
            operator integer_type() const noexcept
            {
                return this->template value_impl<integer_type>();
            }

            //=== operands ===//
            reference& operator+=(integer_type i) noexcept
            {
                return *this = static_cast<integer_type>(value() + i);
            }
            reference& operator-=(integer_type i) noexcept
            {
                return *this = static_cast<integer_type>(value() - i);
            }
            reference& operator*=(integer_type i) noexcept
            {
                return *this = static_cast<integer_type>(value() * i);
            }
            reference& operator/=(integer_type i) noexcept
            {
                return *this = static_cast<integer_type>(value() / i);
            }
            reference& operator%=(integer_type i) noexcept
            {
                return *this = static_cast<integer_type>(value() % i);
            }

            reference& operator++() noexcept
            {
                return *this += 1u;
            }
            integer_type operator++(int) noexcept
            {
                integer_type copy = *this;
                ++*this;
                return copy;
            }
            reference& operator--() noexcept
            {
                return *this -= 1u;
            }
            integer_type operator--(int) noexcept
            {
                integer_type copy = *this;
                --*this;
                return copy;
            }
        };

        template <class... Fields>
        struct const_reference
        : detail::basic_bit_field_reference<const bit_field_unsigned<Tag, SizeInBits>, Fields...>
        {
            using detail::basic_bit_field_reference<const bit_field_unsigned<Tag, SizeInBits>,
                                                    Fields...>::basic_bit_field_reference;

            /// \returns The value of the bit field.
            integer_type value() const noexcept
            {
                return this->template value_impl<integer_type>();
            }

            /// \returns `value()`.
            operator integer_type() const noexcept
            {
                return this->template value_impl<integer_type>();
            }
        };
    };

    /// A bit field that stores a signed integer with the given number of bits.
    template <typename Tag, std::size_t SizeInBits>
    struct bit_field_signed : std::integral_constant<std::size_t, SizeInBits>
    {
        using integer_type = typename std::make_signed<detail::uint_least_n_t<SizeInBits>>::type;

        static constexpr integer_type min()
        {
            return static_cast<integer_type>(-(1ll << (SizeInBits - 1)));
        }

        static constexpr integer_type max() noexcept
        {
            return static_cast<integer_type>((1ll << (SizeInBits - 1)) - 1);
        }

    private:
        using unsigned_type = typename std::make_unsigned<integer_type>::type;

        static integer_type unsigned_to_signed(unsigned_type unsigned_rep) noexcept
        {
            // note: assuming two's complement here
            // take the unsigned rep and look at the most significant bit
            if ((unsigned_rep & (1ull << (SizeInBits - 1))) != 0)
            {
                // bit set, negative value
                // take complement and add one to get the absolute value
                // and clear any overflow bits
                auto absolute_value = static_cast<unsigned_type>(~unsigned_rep + 1);
                // clear any overflow bits
                absolute_value = static_cast<unsigned_type>(
                    bit_view<unsigned_type, 0, SizeInBits>(absolute_value).extract());
                // return absolute value properly negated
                return static_cast<integer_type>(-static_cast<integer_type>(absolute_value));
            }
            else
                // bit not set, positive value, just return
                return static_cast<integer_type>(unsigned_rep);
        }

    public:
        template <class... Fields>
        struct reference
        : detail::basic_bit_field_reference<bit_field_signed<Tag, SizeInBits>, Fields...>
        {
            using detail::basic_bit_field_reference<bit_field_signed<Tag, SizeInBits>,
                                                    Fields...>::basic_bit_field_reference;

            /// \effects Assigns the given integer value to the bit field.
            /// \requires The integer value does not overflow.
            reference& operator=(integer_type i) noexcept
            {
                DEBUG_ASSERT(i >= min() && i <= max(), detail::precondition_handler{},
                             "integer overflow in bit field");

                // take the unsigned representation and truncate it down
                auto unsigned_rep = reinterpret_cast<unsigned_type&>(i);
                this->assign_truncating_impl(unsigned_rep);

                return *this;
            }

            /// \returns The value of the bit field.
            integer_type value() const noexcept
            {
                auto unsigned_rep = this->template value_impl<unsigned_type>();
                return unsigned_to_signed(unsigned_rep);
            }

            /// \returns `value()`.
            operator integer_type() const noexcept
            {
                return value();
            }

            //=== operands ===//
            reference& operator+=(integer_type i) noexcept
            {
                return *this = static_cast<integer_type>(value() + i);
            }
            reference& operator-=(integer_type i) noexcept
            {
                return *this = static_cast<integer_type>(value() - i);
            }
            reference& operator*=(integer_type i) noexcept
            {
                return *this = static_cast<integer_type>(value() * i);
            }
            reference& operator/=(integer_type i) noexcept
            {
                return *this = static_cast<integer_type>(value() / i);
            }
            reference& operator%=(integer_type i) noexcept
            {
                return *this = static_cast<integer_type>(value() % i);
            }

            reference& operator++() noexcept
            {
                return *this += 1;
            }
            integer_type operator++(int) noexcept
            {
                integer_type copy = *this;
                ++*this;
                return copy;
            }
            reference& operator--() noexcept
            {
                return *this -= 1;
            }
            integer_type operator--(int) noexcept
            {
                integer_type copy = *this;
                --*this;
                return copy;
            }
        };

        template <class... Fields>
        struct const_reference
        : detail::basic_bit_field_reference<const bit_field_signed<Tag, SizeInBits>, Fields...>
        {
            using detail::basic_bit_field_reference<const bit_field_signed<Tag, SizeInBits>,
                                                    Fields...>::basic_bit_field_reference;

            /// \returns The value of the bit field.
            integer_type value() const noexcept
            {
                auto unsigned_rep = this->template value_impl<unsigned_type>();
                return unsigned_to_signed(unsigned_rep);
            }

            /// \returns `value()`.
            operator integer_type() const noexcept
            {
                return value();
            }
        };
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_BIT_FIELD_HPP_INCLUDED
