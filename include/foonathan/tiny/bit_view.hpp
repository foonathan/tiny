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

            template <typename Integer>
            constexpr Integer get_mask(std::size_t begin, std::size_t length) noexcept
            {
                return length == sizeof(Integer) * CHAR_BIT ?
                           Integer(-1) :
                           ((Integer(1) << length) - Integer(1)) << begin;
            }

            template <typename Integer, std::size_t Index, std::size_t BeginBit, std::size_t EndBit>
            struct bit_single_extracter
            {
                static constexpr auto mask = get_mask<Integer>(BeginBit, EndBit - BeginBit);

                static std::uintmax_t extract(const Integer* pointer) noexcept
                {
                    return static_cast<std::uintmax_t>((pointer[Index] & mask) >> BeginBit);
                }

                static void put(Integer* pointer, std::uintmax_t bits) noexcept
                {
                    pointer[Index] &= ~mask;
                    pointer[Index] |= static_cast<Integer>((bits << BeginBit) & mask);
                }
            };

            // only for multiple indices!
            template <typename Integer, std::size_t BeginIndex, std::size_t BeginBit,
                      std::size_t EndIndex, std::size_t EndBit, std::size_t I>
            struct bit_loop_extracter
            {
                static constexpr auto bits_used = sizeof(Integer) * CHAR_BIT;

                using tail =
                    bit_loop_extracter<Integer, BeginIndex, BeginBit, EndIndex, EndBit, I + 1>;

                static std::uintmax_t extract(const Integer* pointer) noexcept
                {
                    auto head_result = static_cast<std::uintmax_t>(pointer[I]);
                    auto tail_result = tail::extract(pointer);
                    return (tail_result << bits_used) | head_result;
                }

                static void put(Integer* pointer, std::uintmax_t bits) noexcept
                {
                    pointer[I] = static_cast<Integer>(bits);
                    tail::put(pointer, bits >> bits_used);
                }
            };

            template <typename Integer, std::size_t BeginIndex, std::size_t BeginBit,
                      std::size_t EndIndex, std::size_t EndBit>
            struct bit_loop_extracter<Integer, BeginIndex, BeginBit, EndIndex, EndBit, BeginIndex>
            {
                static constexpr auto bits_used = sizeof(Integer) * CHAR_BIT - BeginBit;
                static constexpr auto mask      = get_mask<Integer>(BeginBit, bits_used);

                static std::uintmax_t extract_head(const Integer* pointer) noexcept
                {
                    return static_cast<std::uintmax_t>((pointer[BeginIndex] & mask) >> BeginBit);
                }

                static void put_head(Integer* pointer, std::uintmax_t bits) noexcept
                {
                    pointer[BeginIndex] &= ~mask;
                    pointer[BeginIndex] |= static_cast<Integer>((bits << BeginBit) & mask);
                }

                using tail = bit_loop_extracter<Integer, BeginIndex, BeginBit, EndIndex, EndBit,
                                                BeginIndex + 1>;

                static std::uintmax_t extract(const Integer* pointer) noexcept
                {
                    auto head_result = extract_head(pointer);
                    auto tail_result = tail::extract(pointer);
                    return (tail_result << bits_used) | head_result;
                }

                static void put(Integer* pointer, std::uintmax_t bits) noexcept
                {
                    put_head(pointer, bits);
                    tail::put(pointer, bits >> bits_used);
                }
            };

            template <typename Integer, std::size_t BeginIndex, std::size_t BeginBit,
                      std::size_t EndIndex, std::size_t EndBit>
            struct bit_loop_extracter<Integer, BeginIndex, BeginBit, EndIndex, EndBit, EndIndex>
            {
                static constexpr auto mask = get_mask<Integer>(0, EndBit);

                static std::uintmax_t extract(const Integer* pointer) noexcept
                {
                    return static_cast<std::uintmax_t>(pointer[EndIndex] & mask);
                }

                static void put(Integer* pointer, std::uintmax_t bits) noexcept
                {
                    pointer[EndIndex] &= ~mask;
                    pointer[EndIndex] |= static_cast<Integer>(bits & mask);
                }
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
            /// \effects Creates a view of the given integer.
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
                return extracter::extract(pointer_);
            }

            /// \effects Sets the viewed bits to the `size()` lower bits of `bits`.
            void put(std::uintmax_t bits) const noexcept
            {
                extracter::put(pointer_, bits);
            }

        private:
            using extracter = detail::bit_single_extracter<unsigned_integer, 0, begin(), end()>;

            unsigned_integer* pointer_;

            template <typename, std::size_t, std::size_t>
            friend class bit_view;
        };

        /// A specialization of [tiny::bit_view]() that can view into an integer array,
        template <typename Integer, std::size_t N, std::size_t Begin, std::size_t End>
        class bit_view<Integer[N], Begin, End>
        {
            static constexpr auto bits_per_element = sizeof(Integer) * CHAR_BIT;

            static_assert(std::is_integral<Integer>::value, "must be an integer type");
            static_assert(Begin <= End, "invalid range");
            static_assert(End <= N * bits_per_element, "out of bounds");
            static_assert(End - Begin <= sizeof(std::uintmax_t) * CHAR_BIT,
                          "too many bits to view at once");

            static constexpr std::size_t array_index(std::size_t i) noexcept
            {
                // if the array element is out of range,
                // subtract one and make bits out of range instead
                return i / bits_per_element == N ? N - 1 : i / bits_per_element;
            }

            static constexpr std::size_t bit_index(std::size_t i) noexcept
            {
                // if the array element is out of range,
                // make bits out of range
                return i / bits_per_element == N ? bits_per_element : i % bits_per_element;
            }

            // range of array elements is [begin_index, end_index], both inclusive
            static constexpr auto begin_index = array_index(Begin);
            static constexpr auto end_index   = array_index(End);

            // range of bits is exclusive
            static constexpr auto begin_bit_index = bit_index(Begin);
            static constexpr auto end_bit_index   = bit_index(End);

            using unsigned_integer = typename std::make_unsigned<Integer>::type;

        public:
            /// \effects Creates a view of the given integer array.
            explicit bit_view(Integer (&array)[N]) noexcept
            : pointer_(reinterpret_cast<unsigned_integer*>(array))
            {
            }

            /// \effects Creates a view from the non-const version.
            template <typename U, typename = typename std::enable_if<
                                      std::is_same<const U[N], Integer[N]>::value>::type>
            bit_view(bit_view<U[N], Begin, End> other) : pointer_(other.pointer_)
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
                auto offset_i = i + Begin;
                return detail::bit_reference<unsigned_integer>(&pointer_[array_index(offset_i)],
                                                               bit_index(offset_i));
            }

            /// \returns An integer containing the viewed bits in the `size()` lower bits.
            std::uintmax_t extract() const noexcept
            {
                return extract(std::integral_constant<bool, begin_index != end_index>{});
            }

            /// \effects Sets the viewed bits to the `size()` lower bits of `bits`.
            void put(std::uintmax_t bits) const noexcept
            {
                put(std::integral_constant<bool, begin_index != end_index>{}, bits);
            }

        private:
            std::uintmax_t extract(std::false_type /* single element */) const noexcept
            {
                return detail::bit_single_extracter<unsigned_integer, begin_index, begin_bit_index,
                                                    end_bit_index>::extract(pointer_);
            }

            void put(std::false_type /* single element */, std::uintmax_t bits) const noexcept
            {
                detail::bit_single_extracter<unsigned_integer, begin_index, begin_bit_index,
                                             end_bit_index>::put(pointer_, bits);
            }

            std::uintmax_t extract(std::true_type /* multiple elements */) const noexcept
            {
                return detail::bit_loop_extracter<unsigned_integer, begin_index, begin_bit_index,
                                                  end_index, end_bit_index,
                                                  begin_index>::extract(pointer_);
            }

            void put(std::true_type /* multiple elements */, std::uintmax_t bits) const noexcept
            {
                return detail::bit_loop_extracter<unsigned_integer, begin_index, begin_bit_index,
                                                  end_index, end_bit_index,
                                                  begin_index>::put(pointer_, bits);
            }

            unsigned_integer* pointer_;

            template <typename, std::size_t, std::size_t>
            friend class bit_view;
        };
    } // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_BIT_VIEW_HPP_INCLUDED
