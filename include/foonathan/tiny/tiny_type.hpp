// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TINY_TYPE_HPP_INCLUDED
#define FOONATHAN_TINY_TINY_TYPE_HPP_INCLUDED

#include <foonathan/tiny/bit_view.hpp>
#include <foonathan/tiny/detail/assert.hpp>

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
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TINY_TYPE_HPP_INCLUDED
