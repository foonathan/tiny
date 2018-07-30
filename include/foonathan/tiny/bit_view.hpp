// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_BIT_VIEW_HPP_INCLUDED
#define FOONATHAN_TINY_BIT_VIEW_HPP_INCLUDED

#include <climits>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <foonathan/tiny/detail/assert.hpp>

namespace foonathan
{
    namespace tiny
    {
        namespace detail
        {
            template <typename Integer>
            class bit_reference
            {
                static_assert(std::is_integral<Integer>::value && std::is_unsigned<Integer>::value,
                              "must be an unsigned integer type");

            public:
                explicit bit_reference(Integer* ptr, std::size_t index) noexcept
                : pointer_(ptr), index_(index)
                {
                }

                bit_reference(bit_reference&&) noexcept = default;
                bit_reference& operator=(bit_reference&&) = delete;

                explicit operator bool() const noexcept
                {
                    return (*pointer_ >> index_) & Integer(1);
                }

                const bit_reference& operator=(bool value) const noexcept
                {
                    // clear
                    *pointer_ &= ~(Integer(1) << index_);
                    // set
                    *pointer_ |= Integer(value) << index_;

                    return *this;
                }

            private:
                Integer*    pointer_;
                std::size_t index_;
            };
        } // namespace detail

        /// A view to the given range of bits in an integer type.
        template <typename Integer, std::size_t Begin, std::size_t End>
        class bit_view
        {
            static_assert(std::is_integral<Integer>::value, "must be an integer type");
            static_assert(Begin <= End, "invalid range");
            static_assert(End <= sizeof(Integer) * CHAR_BIT, "out of bounds");

            using unsigned_integer = typename std::make_unsigned<Integer>::type;

        public:
            /// \effects Creates a view of the given integer type.
            explicit bit_view(Integer& integer) noexcept
            : pointer_(reinterpret_cast<unsigned_integer*>(&integer))
            {
            }

            /// \effects Creates a view from the non-const version.
            template <typename U, typename = typename std::enable_if<
                                      std::is_same<const U, Integer>::value>::type>
            bit_view(bit_view<U, Begin, End> other) : pointer_(other.pointer_)
            {
            }

            /// \returns The begin index.
            static constexpr std::size_t begin() noexcept
            {
                return Begin;
            }

            /// \returns The end index.
            static constexpr std::size_t end() noexcept
            {
                return End;
            }

            /// \returns The number of bits.
            static constexpr std::size_t size() noexcept
            {
                return End - Begin;
            }

            /// \returns A boolean reference to the given bit.
            /// \requires `i < size()`.
            detail::bit_reference<unsigned_integer> operator[](std::size_t i) const noexcept
            {
                DEBUG_ASSERT(i < size(), detail::precondition_handler{}, "index out of range");
                return detail::bit_reference<unsigned_integer>(pointer_, Begin + i);
            }

            /// \returns An integer containing the viewed bits in the `size()` lower bits.
            std::uintmax_t extract() const noexcept
            {
                return (*pointer_ & mask()) >> Begin;
            }

            /// \effects Sets the viewed bits to the `size()` lower bits of `bits`.
            void put(std::uintmax_t bits) const noexcept
            {
                // clear the existing bits
                *pointer_ &= ~mask();
                // set the new ones
                *pointer_ |= static_cast<unsigned_integer>((bits << Begin) & mask());
            }

        private:
            static constexpr Integer mask() noexcept
            {
                return size() == sizeof(Integer) * CHAR_BIT ?
                           Integer(-1) :
                           ((Integer(1) << size()) - Integer(1)) << Begin;
            }

            unsigned_integer* pointer_;

            template <typename, std::size_t, std::size_t>
            friend class bit_view;
        };
    } // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_BIT_VIEW_HPP_INCLUDED
