// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TINY_TYPE_STORAGE_HPP_INCLUDED
#define FOONATHAN_TINY_TINY_TYPE_STORAGE_HPP_INCLUDED

#include <foonathan/tiny/detail/index_sequence.hpp>
#include <foonathan/tiny/tiny_type.hpp>

namespace foonathan
{
namespace tiny
{
    /// Tags a tiny type for easy retrieval in the [lex::tiny_type_storage]().
    template <class Tag, class TinyType>
    struct tiny_tagged
    {
        using object_type = typename TinyType::object_type;

        static constexpr std::size_t bit_size() noexcept
        {
            return TinyType::bit_size();
        }

        template <class BitView>
        using proxy = typename TinyType::template proxy<BitView>;
    };

    namespace tiny_storage_detail
    {
        //=== storage_type ===//
        template <class... TinyTypes>
        struct total_size;

        template <>
        struct total_size<> : std::integral_constant<std::size_t, 0>
        {};

        template <class Head, class... Tail>
        struct total_size<Head, Tail...>
        : std::integral_constant<std::size_t, Head::bit_size() + total_size<Tail...>::value>
        {};

        template <class... TinyTypes>
        using storage_type
            = unsigned char[total_size<TinyTypes...>::value / CHAR_BIT
                            + (total_size<TinyTypes...>::value % CHAR_BIT == 0 ? 0 : 1)];

        //=== is_target ===//
        template <std::size_t Index, typename Target, class TinyType>
        struct is_target : std::false_type
        {};

        template <std::size_t Index, typename Target>
        struct is_target<Index, Target, Target> : std::true_type
        {};

        template <std::size_t Index, typename Tag, class TinyType>
        struct is_target<Index, Tag, tiny_tagged<Tag, TinyType>> : std::true_type
        {};

        template <std::size_t Index, class TinyType>
        struct is_target<Index, std::integral_constant<std::size_t, Index>, TinyType>
        : std::true_type
        {};

        //=== bit_view_of ===//
        template <typename Target, std::size_t CurIndex, std::size_t CurOffset, class... TinyTypes>
        struct find_impl
        {
            static constexpr auto found_count = 0;
            static constexpr auto offset      = 0;
            using type                        = tiny_bool; // just some type fulfilling the concept
        };

        template <typename Target, std::size_t CurIndex, std::size_t CurOffset, class Head,
                  class... Tail>
        struct find_impl<Target, CurIndex, CurOffset, Head, Tail...>
        {
            using recurse = find_impl<Target, CurIndex + 1, CurOffset + Head::bit_size(), Tail...>;

            static constexpr auto found       = is_target<CurIndex, Target, Head>::value;
            static constexpr auto found_count = recurse::found_count + (found ? 1 : 0);

            static constexpr auto offset = found ? CurOffset : recurse::offset;
            using type = typename std::conditional<found, Head, typename recurse::type>::type;
        };

        template <typename Target, class... TinyTypes>
        struct find
        {
            using result = find_impl<Target, 0, 0, TinyTypes...>;
            static_assert(result::found_count > 0, "tiny type not stored");
            static_assert(result::found_count <= 1,
                          "tiny type ambiguous, use index or tagged type");

            using type = typename result::type;

            using view  = bit_view<storage_type<TinyTypes...>, result::offset,
                                  result::offset + type::bit_size()>;
            using cview = bit_view<const storage_type<TinyTypes...>, result::offset,
                                   result::offset + type::bit_size()>;

            using proxy  = typename type::template proxy<view>;
            using cproxy = typename type::template proxy<cview>;
        };

        template <typename Target, class... TinyTypes>
        using tiny_type = typename find<Target, TinyTypes...>::type;

        template <typename Target, class... TinyTypes>
        using view_of = typename find<Target, TinyTypes...>::view;
        template <typename Target, class... TinyTypes>
        using cview_of = typename find<Target, TinyTypes...>::cview;

        template <typename Target, class... TinyTypes>
        using proxy_of = typename find<Target, TinyTypes...>::proxy;
        template <typename Target, class... TinyTypes>
        using cproxy_of = typename find<Target, TinyTypes...>::cproxy;
    } // namespace tiny_storage_detail

    template <class... TinyTypes>
    class tiny_type_storage
    {
    public:
        //=== constructors ===//
        /// Default constructor.
        /// \effects Initializes all tiny types to the value corresponding to all zeroes.
        tiny_type_storage() noexcept : storage_{} {}

        /// Object constructor.
        /// \effects Initializes all tiny types from the corresponding object type.
        tiny_type_storage(typename TinyTypes::object_type... objects) noexcept
        : tiny_type_storage(detail::make_index_sequence<sizeof...(TinyTypes)>{}, objects...)
        {}

        //=== access ===//
        /// Array access operator.
        /// \returns `get<Tag>()`.
        /// \group array
        template <class Tag>
        auto operator[](Tag) noexcept -> tiny_storage_detail::proxy_of<Tag, TinyTypes...>
        {
            return make_tiny_proxy<tiny_storage_detail::tiny_type<Tag, TinyTypes...>>(
                tiny_storage_detail::view_of<Tag, TinyTypes...>(storage_));
        }
        /// \group array
        template <class Tag>
        auto operator[](Tag) const noexcept -> tiny_storage_detail::cproxy_of<Tag, TinyTypes...>
        {
            return make_tiny_proxy<tiny_storage_detail::tiny_type<Tag, TinyTypes...>>(
                tiny_storage_detail::cview_of<Tag, TinyTypes...>(storage_));
        }

        /// \returns `get<std::integral_constant<std::size_t, I>>()`.
        /// \group get_index
        template <std::size_t I>
        auto get() noexcept
            -> tiny_storage_detail::proxy_of<std::integral_constant<std::size_t, I>, TinyTypes...>
        {
            return make_tiny_proxy<tiny_storage_detail::tiny_type<
                std::integral_constant<std::size_t, I>, TinyTypes...>>(
                tiny_storage_detail::view_of<std::integral_constant<std::size_t, I>, TinyTypes...>(
                    storage_));
        }
        /// \group get_index
        template <std::size_t I>
        auto get() const noexcept
            -> tiny_storage_detail::cproxy_of<std::integral_constant<std::size_t, I>, TinyTypes...>
        {
            return make_tiny_proxy<tiny_storage_detail::tiny_type<
                std::integral_constant<std::size_t, I>, TinyTypes...>>(
                tiny_storage_detail::cview_of<std::integral_constant<std::size_t, I>, TinyTypes...>(
                    storage_));
        }

        /// \returns The proxy of the tiny type corresponding to `Tag`.
        /// `Tag` can be `std::integral_constant<std::size_t, I>`, which matches to the `I`th type,
        /// one of the exact types or the tag from [lex::tiny_tagged]().
        /// \group get
        template <class Tag>
        auto get() noexcept -> tiny_storage_detail::proxy_of<Tag, TinyTypes...>
        {
            return make_tiny_proxy<tiny_storage_detail::tiny_type<Tag, TinyTypes...>>(
                tiny_storage_detail::view_of<Tag, TinyTypes...>(storage_));
        }
        /// \group get
        template <class Tag>
        auto get() const noexcept -> tiny_storage_detail::cproxy_of<Tag, TinyTypes...>
        {
            return make_tiny_proxy<tiny_storage_detail::tiny_type<Tag, TinyTypes...>>(
                tiny_storage_detail::cview_of<Tag, TinyTypes...>(storage_));
        }

    private:
        template <std::size_t... Indices>
        tiny_type_storage(detail::index_sequence<Indices...>,
                          typename TinyTypes::object_type... objects)
        : storage_{}
        {
            bool for_each[] = {(get<Indices>() = objects, true)..., true};
            (void)for_each;
        }

        tiny_storage_detail::storage_type<TinyTypes...> storage_;
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TINY_TYPE_STORAGE_HPP_INCLUDED
