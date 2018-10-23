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
        //=== bit_reference ===//
        class bit_reference
        {
        public:
            template <typename Integer>
            explicit bit_reference(Integer* pointer, std::size_t index) noexcept
            : modifier_([](void* ptr_, std::size_t index, bool value) {
                  auto ptr = static_cast<Integer*>(ptr_);
                  // clear
                  *ptr &= ~(Integer(1) << index);
                  // set
                  *ptr |= Integer(value) << index;
              }),
              pointer_(pointer), index_(index), value_((*pointer >> index_) & Integer(1))
            {
                static_assert(std::is_integral<Integer>::value && std::is_unsigned<Integer>::value,
                              "must be an unsigned integer type");
            }

            bit_reference(bit_reference&&) noexcept = default;

            bit_reference& operator=(bit_reference&&) = delete;

            explicit operator bool() const noexcept
            {
                return value_;
            }

            const bit_reference& operator=(bool value) const noexcept
            {
                value_ = value;
                modifier_(pointer_, index_, value);
                return *this;
            }

        private:
            using modifier = void (*)(void*, std::size_t, bool);

            modifier     modifier_;
            void*        pointer_;
            std::size_t  index_;
            mutable bool value_;
        };

        template <bool IsConst>
        using bit_reference_for_impl =
            typename std::conditional<IsConst, bool, bit_reference>::type;
        template <typename Integer>
        using bit_reference_for = bit_reference_for_impl<std::is_const<Integer>::value>;

        template <typename Integer>
        bit_reference make_bit_reference(Integer* pointer, std::size_t index) noexcept
        {
            return bit_reference(pointer, index);
        }
        template <typename Integer>
        bool make_bit_reference(const Integer* pointer, std::size_t index) noexcept
        {
            return static_cast<bool>((*pointer >> index) & Integer(1));
        }

        //=== extracter ===//
        template <typename Integer>
        constexpr Integer get_mask(std::size_t begin, std::size_t length) noexcept
        {
            return length == sizeof(Integer) * CHAR_BIT
                       ? Integer(-1)
                       : static_cast<Integer>(((1ull << length) - 1ull) << begin);
        }

        template <typename Integer, std::size_t Index, std::size_t BeginBit, std::size_t EndBit>
        struct bit_single_extracter
        {
            static constexpr auto mask = get_mask<Integer>(BeginBit, EndBit - BeginBit);

            static std::uintmax_t extract(const Integer* pointer) noexcept
            {
                return static_cast<std::uintmax_t>(
                    (static_cast<std::uintmax_t>(pointer[Index]) & mask) >> BeginBit);
            }

            static void put(Integer* pointer, std::uintmax_t bits) noexcept
            {
                pointer[Index] &= static_cast<Integer>(~mask);
                pointer[Index] |= static_cast<Integer>((bits << BeginBit) & mask);
            }
        };

        // only for multiple indices!
        template <typename Integer, std::size_t BeginIndex, std::size_t BeginBit,
                  std::size_t EndIndex, std::size_t EndBit, std::size_t I>
        struct bit_loop_extracter
        {
            static constexpr auto bits_used = sizeof(Integer) * CHAR_BIT;

            using tail = bit_loop_extracter<Integer, BeginIndex, BeginBit, EndIndex, EndBit, I + 1>;

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
                pointer[BeginIndex] &= static_cast<Integer>(~mask);
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
                pointer[EndIndex] &= static_cast<Integer>(~mask);
                pointer[EndIndex] |= static_cast<Integer>(bits & mask);
            }
        };
    } // namespace detail

    /// Constant to mark the remaining bits in an integer.
    ///
    /// `bit_view<int32_t, 0, last_bit>` is equivalent to `bit_view<int32_t, 0, 32>`.
    constexpr std::size_t last_bit = std::size_t(-1);

    /// A view to the given range `[Begin, End)` of bits in an integer type.
    template <typename Integer, std::size_t Begin, std::size_t End>
    class bit_view
    {
        static_assert(std::is_integral<Integer>::value, "must be an integer type");
        static_assert(Begin <= End, "invalid range");
        static_assert(End == last_bit || End <= sizeof(Integer) * CHAR_BIT, "out of bounds");

        using unsigned_integer = typename std::make_unsigned<Integer>::type;
        using is_const         = std::is_const<Integer>;

    public:
        /// \returns The begin index.
        static constexpr std::size_t begin() noexcept
        {
            return Begin == last_bit ? sizeof(Integer) * CHAR_BIT : Begin;
        }

        /// \returns The end index.
        static constexpr std::size_t end() noexcept
        {
            return End == last_bit ? sizeof(Integer) * CHAR_BIT : End;
        }

        /// \returns The number of bits.
        static constexpr std::size_t size() noexcept
        {
            return end() - begin();
        }

        /// \effects Creates a view of the given integer.
        explicit bit_view(Integer& integer) noexcept
        : pointer_(reinterpret_cast<unsigned_integer*>(&integer))
        {}

        /// \effects Creates a view from the non-const version.
        template <typename U,
                  typename = typename std::enable_if<std::is_same<const U, Integer>::value>::type>
        bit_view(bit_view<U, Begin, End> other) noexcept : pointer_(other.pointer_)
        {}

        /// \returns A boolean reference to the given bit.
        /// \requires `i < size()`.
        /// \notes The index is in the range `[0, size())`, where `0` is the `Begin` bit.
        detail::bit_reference_for<unsigned_integer> operator[](std::size_t i) const noexcept
        {
            DEBUG_ASSERT(i < size(), detail::precondition_handler{}, "index out of range");
            return detail::make_bit_reference(pointer_, Begin + i);
        }

        /// \returns An integer containing the viewed bits in the `size()` lower bits.
        std::uintmax_t extract() const noexcept
        {
            return extracter::extract(pointer_);
        }

        /// \effects Sets the viewed bits to the `size()` lower bits of `bits`.
        /// \param T
        /// \exclude
        template <typename T = Integer>
        void put(std::uintmax_t bits) const noexcept
        {
            static_assert(!std::is_const<T>::value, "cannot put in a view to const");
            extracter::put(pointer_, bits);
        }

    private:
        using extracter = detail::bit_single_extracter<unsigned_integer, 0, begin(), end()>;

        unsigned_integer* pointer_;

        template <typename, std::size_t, std::size_t>
        friend class bit_view;
    };

    /// A specialization of [tiny::bit_view]() that can view into an integer array.
    template <typename Integer, std::size_t N, std::size_t Begin, std::size_t End>
    class bit_view<Integer[N], Begin, End>
    {
        static constexpr auto bits_per_element = sizeof(Integer) * CHAR_BIT;

        using is_const = std::is_const<Integer>;

    public:
        /// \returns The begin index.
        static constexpr std::size_t begin() noexcept
        {
            return Begin == last_bit ? N * bits_per_element : Begin;
        }

        /// \returns The end index.
        static constexpr std::size_t end() noexcept
        {
            return End == last_bit ? N * bits_per_element : End;
        }

        /// \returns The number of bits.
        static constexpr std::size_t size() noexcept
        {
            return end() - begin();
        }

    private:
        static_assert(std::is_integral<Integer>::value, "must be an integer type");
        static_assert(begin() <= end(), "invalid range");
        static_assert(End == last_bit || End <= N * bits_per_element, "out of bounds");
        static_assert(size() <= sizeof(std::uintmax_t) * CHAR_BIT, "too many bits to view at once");

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
        static constexpr auto begin_index = array_index(begin());
        static constexpr auto end_index   = array_index(end());

        // range of bits is exclusive
        static constexpr auto begin_bit_index = bit_index(begin());
        static constexpr auto end_bit_index   = bit_index(end());

        using unsigned_integer = typename std::make_unsigned<Integer>::type;

    public:
        /// \effects Creates a view of the given integer array.
        explicit bit_view(Integer (&array)[N]) noexcept
        : pointer_(reinterpret_cast<unsigned_integer*>(array))
        {}

        /// \effects Creates a view from the non-const version.
        template <typename U,
                  typename
                  = typename std::enable_if<std::is_same<const U[N], Integer[N]>::value>::type>
        bit_view(bit_view<U[N], Begin, End> other) noexcept : pointer_(other.pointer_)
        {}

        /// \returns A boolean reference to the given bit.
        /// \requires `i < size()`.
        /// \notes The index is in the range `[0, size())`, where `0` is the `Begin` bit.
        detail::bit_reference_for<unsigned_integer> operator[](std::size_t i) const noexcept
        {
            DEBUG_ASSERT(i < size(), detail::precondition_handler{}, "index out of range");
            auto offset_i = i + begin();
            return detail::make_bit_reference(&pointer_[array_index(offset_i)],
                                              bit_index(offset_i));
        }

        /// \returns An integer containing the viewed bits in the `size()` lower bits.
        std::uintmax_t extract() const noexcept
        {
            return extract(std::integral_constant<bool, begin_index != end_index>{});
        }

        /// \effects Sets the viewed bits to the `size()` lower bits of `bits`.
        /// \param T
        /// \exclude
        template <typename T = Integer>
        void put(std::uintmax_t bits) const noexcept
        {
            static_assert(!std::is_const<T>::value, "cannot put in a view to const");
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
                                              end_index, end_bit_index, begin_index>::put(pointer_,
                                                                                          bits);
        }

        unsigned_integer* pointer_;

        template <typename, std::size_t, std::size_t>
        friend class bit_view;
    };

    /// Tag type to create a joined bit view.
    template <class BitView, typename Integer>
    struct joined_bit_view_tag
    {};

    /// Specialization for a bit view that views `[Begin, End)` in `Integer`,
    /// then forwards to `BitView` for the remaining bits.
    /// \notes It is not meant to be used directly,
    /// use [tiny::joined_bit_view]() and [tiny::join_bit_views]() instead.
    template <class BitView, typename Integer, std::size_t Begin, std::size_t End>
    class bit_view<joined_bit_view_tag<BitView, Integer>, Begin, End>
    {
        using is_const = std::integral_constant<bool, BitView::is_const::value
                                                          || std::is_const<Integer>::value>;

    public:
        /// \returns The number of bits.
        static constexpr std::size_t size() noexcept
        {
            return bit_view<Integer, Begin, End>::size() + BitView::size();
        }

        /// \effects Creates a view from existing views.
        template <typename... TailArgs>
        bit_view(bit_view<Integer, Begin, End> head, TailArgs... args) noexcept
        : head_(head), tail_(args...)
        {}

        /// \effects Creates a view for the given parts.
        template <typename... Tail>
        explicit bit_view(Integer& h, Tail&... tail) noexcept : head_(h), tail_(tail...)
        {}

        /// \effects Creates a view from the non-const version.
        template <class OtherBitView, class OtherInteger,
                  typename = typename std::enable_if<
                      std::is_constructible<BitView, OtherBitView>::value
                      && std::is_same<const OtherInteger, Integer>::value>::type>
        bit_view(
            bit_view<joined_bit_view_tag<OtherBitView, OtherInteger>, Begin, End> other) noexcept
        : head_(other.head_), tail_(other.tail_)
        {}

        /// \returns A boolean reference to the given bit.
        /// \requires `i < size()`.
        /// \notes The index is in the range `[0, size())`, where `0` is the first bit.
        detail::bit_reference_for_impl<is_const::value> operator[](std::size_t i) const noexcept
        {
            DEBUG_ASSERT(i < size(), detail::precondition_handler{}, "index out of range");
            if (i < head_.size())
                return detail::bit_reference_for_impl<is_const::value>(head_[i]);
            else
                return detail::bit_reference_for_impl<is_const::value>(tail_[i - head_.size()]);
        }

        /// \returns An integer containing the viewed bits in the `size()` lower bits.
        std::uintmax_t extract() const noexcept
        {
            static_assert(size() <= sizeof(std::uintmax_t) * CHAR_BIT, "viewing too many bits");

            auto head = head_.extract();
            auto tail = tail_.extract();
            return (tail << head_.size()) | head;
        }

        /// \effects Sets the viewed bits to the `size()` lower bits of `bits`.
        /// \param T
        /// \exclude
        template <typename T = Integer>
        void put(std::uintmax_t bits) const noexcept
        {
            static_assert(!std::is_const<T>::value, "cannot put in a view to const");
            head_.put(bits);
            tail_.put(bits >> head_.size());
        }

    private:
        bit_view<Integer, Begin, End> head_;
        BitView                       tail_;

        template <typename, std::size_t, std::size_t>
        friend class bit_view;
    };

    namespace detail
    {
        template <class... BitViews>
        struct joined_bit_view_impl;

        template <typename Integer, std::size_t Begin, std::size_t End, class FirstTail,
                  class... RestTail>
        struct joined_bit_view_impl<bit_view<Integer, Begin, End>, FirstTail, RestTail...>
        {
            using tail_view = typename joined_bit_view_impl<FirstTail, RestTail...>::type;
            using type      = bit_view<joined_bit_view_tag<tail_view, Integer>, Begin, End>;
        };

        template <typename Integer, std::size_t Begin, std::size_t End>
        struct joined_bit_view_impl<bit_view<Integer, Begin, End>>
        {
            using type = bit_view<Integer, Begin, End>;
        };
    } // namespace detail

    /// A bit view that concatenates the other bit views in order.
    ///
    /// It first views the bits from the first view, then from the second, and so on.
    template <class... BitViews>
    using joined_bit_view = typename detail::joined_bit_view_impl<BitViews...>::type;

    /// \returns A joined bit view from the given views.
    template <class... BitViews>
    joined_bit_view<BitViews...> join_bit_views(BitViews... views) noexcept
    {
        return {views...};
    }

    //=== bit_view convenience functions ===//
    /// \returns The specified bit view.
    template <std::size_t Begin, std::size_t End, typename Integer>
    bit_view<Integer, Begin, End> make_bit_view(Integer& i) noexcept
    {
        return bit_view<Integer, Begin, End>(i);
    }

    /// Extracts the specified range of bits from an integer.
    /// \returns `bit_view<Integer, Begin, End>(i).extract()`
    template <std::size_t Begin, std::size_t End, typename Integer>
    std::uintmax_t extract_bits(Integer i) noexcept
    {
        return bit_view<Integer, Begin, End>(i).extract();
    }

    /// Puts bits into the specified range of bits.
    /// \effects Same as `bit_view<Integer, Begin, End>(i).put(bits)`.
    template <std::size_t Begin, std::size_t End, typename Integer>
    void put_bits(Integer& i, std::uintmax_t bits) noexcept
    {
        bit_view<Integer, Begin, End>(i).put(bits);
    }

    /// Checks that the range of bits is zero.
    /// \returns `extract_bits<Begin, End>(i) == 0`.
    template <std::size_t Begin, std::size_t End, typename Integer>
    bool are_cleared_bits(Integer i) noexcept
    {
        return extract_bits<Begin, End>(i) == 0;
    }

    /// Checks that the bits not in the range are zero.
    template <std::size_t Begin, std::size_t End, typename Integer>
    bool are_only_bits(Integer i) noexcept
    {
        return are_cleared_bits<0, Begin>(i) && are_cleared_bits<End, last_bit>(i);
    }

    /// Clears all bits in the specified range by setting them to zero.
    template <std::size_t Begin, std::size_t End, typename Integer>
    void clear_bits(Integer& i) noexcept
    {
        put_bits<Begin, End>(i, 0);
    }

    /// Clears all bits not in the specified range by setting them to zero.
    template <std::size_t Begin, std::size_t End, typename Integer>
    void clear_other_bits(Integer& i) noexcept
    {
        clear_bits<0, Begin>(i);
        clear_bits<End, last_bit>(i);
    }
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_BIT_VIEW_HPP_INCLUDED
