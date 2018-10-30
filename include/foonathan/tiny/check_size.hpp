// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_CHECK_SIZE_HPP_INCLUDED
#define FOONATHAN_TINY_CHECK_SIZE_HPP_INCLUDED

#include <cstddef>

namespace foonathan
{
namespace tiny
{
    /// \exclude
    namespace detail
    {
        template <std::size_t Size>
        struct sizeof_is
        {
            template <std::size_t Expected>
            struct but_expected
            {
                static_assert(Size == Expected, "size wasn't as expected");
                static constexpr bool value = true;
            };
        };

        template <std::size_t Alignment>
        struct alignment_is
        {
            template <std::size_t Expected>
            struct but_expected
            {
                static_assert(Alignment == Expected, "alignment wasn't as expected");
                static constexpr bool value = true;
            };
        };
    } // namespace detail

    /// Checks that `T` has the given `Size`.
    ///
    /// The function will always return `true`, but if `sizeof(T)` isn't` Size,
    /// it will trigger a `static_assert()` where the instantiation will contain the actual value of
    /// `sizeof(T)`.
    ///
    /// You just need to call this function somewhere, but as it returns a boolean,
    /// you can just use a `static_assert(check_size<T, Size>())` after the class definition.
    template <typename T, std::size_t Size>
    constexpr bool check_size() noexcept
    {
        return detail::sizeof_is<sizeof(T)>::template but_expected<Size>::value;
    }

    /// Expands to something equivalent to `static_assert(check_size<T, Size>())`, i.e. checks that
    /// `T` has size `Size`.
#define FOONATHAN_TINY_CHECK_SIZE(T, Size)                                                         \
    static_assert(foonathan::tiny::detail::sizeof_is<sizeof(                                       \
                          T)>::template but_expected<Size>::value                                  \
                      || true,                                                                     \
                  "")

    /// Same as [tiny::check_size]() but for the alignment.
    template <typename T, std::size_t Alignment>
    constexpr bool check_alignment() noexcept
    {
        return detail::alignment_is<alignof(T)>::template but_expected<Alignment>::value;
    }

    /// Same as [FOONATHAN_TINY_CHECK_SIZE]() but for the alignment.
#define FOONATHAN_TINY_CHECK_ALIGNMENT(T, Alignment)                                               \
    static_assert(foonathan::tiny::detail::alignment_is<alignof(                                   \
                          T)>::template but_expected<Alignment>::value                             \
                      || true,                                                                     \
                  "")
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_CHECK_SIZE_HPP_INCLUDED
