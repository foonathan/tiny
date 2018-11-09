// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TINY_FLAG_SET_HPP_INCLUDED
#define FOONATHAN_TINY_TINY_FLAG_SET_HPP_INCLUDED

#include <foonathan/tiny/detail/select_integer.hpp>
#include <foonathan/tiny/enum_traits.hpp>
#include <foonathan/tiny/tiny_type.hpp>

namespace foonathan
{
namespace tiny
{
    namespace detail
    {
        template <typename Enum>
        constexpr std::size_t flag_bit_size() noexcept
        {
            return enum_size<Enum>();
        }

        template <typename Enum>
        using flag_storage_type = detail::uint_least_n_t<flag_bit_size<Enum>()>;

        template <typename Enum>
        constexpr flag_storage_type<Enum> as_flag(Enum e) noexcept
        {
            return flag_storage_type<Enum>(
                1ull << static_cast<typename std::underlying_type<Enum>::type>(e));
        }

        template <typename Enum, typename... Flags>
        detail::flag_storage_type<Enum> combine_flags(Flags... flags) noexcept
        {
            flag_storage_type<Enum> storage = 0;
            bool                    for_each[]
                = {(storage = flag_storage_type<Enum>(storage | as_flag(flags)), true)..., true};
            (void)for_each;
            return storage;
        }
    } // namespace detail

    /// Stores a combination of flags.
    ///
    /// This type is the `object_type` of [tiny::tiny_flag_set](),
    /// which means a tiny flag set can be constructed giving it an initial combination of flags.
    template <typename Enum>
    class flag_combo
    {
        flag_combo(detail::flag_storage_type<Enum> flags) noexcept : flags_(flags) {}

        detail::flag_storage_type<Enum> flags_;

        template <typename E, typename... Flags>
        friend flag_combo<E> flags(Flags... flags) noexcept;
        template <typename FirstFlag, typename... OtherFlags>
        friend flag_combo<FirstFlag> flags(FirstFlag first, OtherFlags... other) noexcept;
        template <typename E>
        friend class tiny_flag_set;
    };

    /// \returns A combination of the specified flags.
    /// \group flags
    template <typename Enum, typename... Flags>
    flag_combo<Enum> flags(Flags... flags) noexcept
    {
        return detail::combine_flags<Enum>(flags...);
    }
    /// \group flags
    template <typename FirstFlag, typename... OtherFlags>
    flag_combo<FirstFlag> flags(FirstFlag first, OtherFlags... other) noexcept
    {
        return detail::combine_flags<FirstFlag>(first, other...);
    }

    /// A tiny set of flags.
    ///
    /// The `Enum` must be an `unsigned` contiguous enum, i.e. it must specialize the
    /// [tiny::enum_traits]() appropriately. Each enum value corresponds to one flag that can either
    /// be set or not set.
    ///
    /// \notes Add a final `flag_count_` or `_count_flag` enum value to get a traits specialization
    /// automatically.
    ///
    /// \notes The enum values must be contigous integers and not powers of two like
    /// when you do a flag set manually.
    template <typename Enum>
    class tiny_flag_set
    {
        using traits    = enum_traits<Enum>;
        using enum_type = typename traits::enum_type;

        static_assert(traits::is_contiguous, "flags must be contiguous");
        static_assert(traits::min() == enum_type(0), "flags must be unsigned");

        static std::size_t get_flag_index(Enum e) noexcept
        {
            return static_cast<std::size_t>(e);
        }

        static std::uintmax_t get_flags(flag_combo<Enum> combo) noexcept
        {
            return combo.flags_;
        }

    public:
        using object_type = flag_combo<Enum>;

        static constexpr std::size_t bit_size() noexcept
        {
            return detail::flag_bit_size<Enum>();
        }

        template <class BitView>
        class proxy
        {
        public:
            /// \effects Assigns the same flags as in the combination.
            const proxy& operator=(flag_combo<Enum> combo) const noexcept
            {
                view_.put(get_flags(combo));
                return *this;
            }

            /// \returns A proxy acting like a boolean reference to the state of the specified flag.
            auto operator[](Enum flag) const noexcept
                -> decltype(std::declval<BitView>()[get_flag_index(flag)])
            {
                return view_[get_flag_index(flag)];
            }

            /// \returns An integer where the `i`th bit is set if the `i`th flag is set.
            std::uintmax_t get() const noexcept
            {
                return view_.extract();
            }

            //=== single flag operation ===//
            /// \returns Whether or not the specified flag is set.
            bool is_set(Enum flag) const noexcept
            {
                return !!view_[get_flag_index(flag)];
            }

            /// \effects Sets the specified flag to `value`.
            void set(Enum flag, bool value) const noexcept
            {
                view_[get_flag_index(flag)] = value;
            }

            /// \effects Sets the specified flag to `true`.
            void set(Enum flag) const noexcept
            {
                set(flag, true);
            }

            /// \effects Sets the specified flag to `false`.
            void reset(Enum flag) const noexcept
            {
                set(flag, false);
            }

            /// \effects Toggles the specified flag.
            void toggle(Enum flag) const noexcept
            {
                set(flag, !is_set(flag));
            }

            //=== multi flag operations ===//
            /// \returns Whether or not any flag is set.
            bool any() const noexcept
            {
                return view_.extract() != 0;
            }

            /// \returns Whether or not all flags are set.
            bool all() const noexcept
            {
                return view_.extract() == clear_other_bits<0, bit_size()>(~0ull);
            }

            /// \returns Whether or not no flags are set.
            bool none() const noexcept
            {
                return view_.extract() == 0;
            }

            /// \effects Sets all flags to `value`.
            void set_all(bool value) const noexcept
            {
                if (value)
                    view_.put(~0ull);
                else
                    view_.put(0);
            }

            /// \effects Sets all flags to `true`.
            void set_all() const noexcept
            {
                set_all(true);
            }

            /// \effects Sets all flags to `false`.
            void reset_all() const noexcept
            {
                set_all(false);
            }

            /// \effects Toggles all flags.
            void toggle_all() const noexcept
            {
                view_.put(~view_.extract());
            }

            //=== comparison ===//
            friend bool operator==(const proxy& lhs, const proxy& rhs) noexcept
            {
                return lhs.get() == rhs.get();
            }
            friend bool operator!=(const proxy& lhs, const proxy& rhs) noexcept
            {
                return lhs.get() != rhs.get();
            }

            bool operator==(flag_combo<Enum> rhs) const noexcept
            {
                return get() == get_flags(rhs);
            }
            bool operator!=(flag_combo<Enum> rhs) const noexcept
            {
                return get() != get_flags(rhs);
            }

            friend bool operator==(flag_combo<Enum> lhs, const proxy& rhs) noexcept
            {
                return rhs == lhs;
            }
            friend bool operator!=(flag_combo<Enum> lhs, const proxy& rhs) noexcept
            {
                return rhs != lhs;
            }

        private:
            explicit proxy(BitView view) noexcept : view_(view) {}

            BitView view_;

            friend tiny_type_access;
        };
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TINY_FLAG_SET_HPP_INCLUDED
