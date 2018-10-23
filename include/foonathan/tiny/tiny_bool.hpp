// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TINY_BOOL_HPP_INCLUDED
#define FOONATHAN_TINY_TINY_BOOL_HPP_INCLUDED

#include <foonathan/tiny/tiny_type.hpp>

namespace foonathan
{
namespace tiny
{
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
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TINY_BOOL_HPP_INCLUDED
