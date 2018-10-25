// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_PADDING_TRAITS_HPP_INCLUDED
#define FOONATHAN_TINY_PADDING_TRAITS_HPP_INCLUDED

#include <cstddef>
#include <foonathan/tiny/bit_view.hpp>

namespace foonathan
{
namespace tiny
{
    //=== padding_traits ===//
    /// Information about the padding bits of a type.
    ///
    /// A specialization usually simply inherits from [tiny::padding_traits_aggregate]().
    template <typename T>
    struct padding_traits
    {
        /// Whether or not this is a specialization that actually provides information.
        static constexpr auto is_specialized = false;

        /// \returns A [tiny::bit_view]() to the padding bytes.
        /// The integer type must be `unsigned char[sizeof(T)]`.
        /// \group padding_view
        static bit_view<unsigned char[sizeof(T)], 0, 0> padding_view(T& object) noexcept
        {
            (void)object;
            return {};
        }
        /// \group padding_view
        static bit_view<const unsigned char[sizeof(T)], 0, 0> padding_view(const T& object) noexcept
        {
            (void)object;
            return {};
        }
    };

    /// \returns The padding view type of the given type.
    /// \notes Its `const`-qualification depends on the cv of `T`.
    template <typename T>
    using padding_view_t = decltype(
        padding_traits<typename std::remove_cv<T>::type>::padding_view(std::declval<T&>()));

    /// \returns The amount of padding bits in the given type.
    template <typename T>
    constexpr std::size_t padding_of() noexcept
    {
        return padding_view_t<T>::size();
    }

    //=== padding_traits_aggregate ===//
    /// Traits type containing information about a member of an aggregate type.
    template <class T, typename Member, Member(T::*Ptr), std::size_t Offset>
    struct aggregate_member
    {
        using object_type = T;
        using member_type = Member;

        /// \returns The offset of that member inside `T`.
        static constexpr std::size_t offset() noexcept
        {
            return Offset;
        }

        /// \returns A reference to the member.
        /// \group get
        static Member& get(T& obj) noexcept
        {
            return obj.*Ptr;
        }
        /// \group get
        static const Member& get(const T& obj) noexcept
        {
            return obj.*Ptr;
        }
    };

    /// Creates a [tiny::aggregate_member]() to the member `Name` of type `Type`.
#define FOONATHAN_TINY_MEMBER(Type, Name)                                                          \
    aggregate_member<Type, decltype(std::declval<Type>().Name), &Type::Name, offsetof(Type, Name)>

    namespace detail
    {
        template <class FirstMember, class SecondMember>
        struct padding_between
        {
            static constexpr auto begin
                = (FirstMember::offset() + sizeof(typename FirstMember::member_type)) * CHAR_BIT;
            static constexpr auto end = SecondMember::offset() * CHAR_BIT;

            static constexpr auto has_padding = begin != end;

            template <class ObjectView>
            static auto get_subview(ObjectView object_view) noexcept
                -> decltype(object_view.template subview<begin, end>())
            {
                return object_view.template subview<begin, end>();
            }
        };

        template <class Member>
        struct padding_between<Member, void>
        {
            static constexpr auto begin
                = (Member::offset() + sizeof(typename Member::member_type)) * CHAR_BIT;
            static constexpr auto end = sizeof(typename Member::object_type) * CHAR_BIT;

            static constexpr auto has_padding = begin != end;

            template <class ObjectView>
            static auto get_subview(ObjectView object_view) noexcept
                -> decltype(object_view.template subview<begin, end>())
            {
                return object_view.template subview<begin, end>();
            }
        };

        template <class... Members>
        struct member_list
        {};

        // MembersB is MembersA shifted over by one and with a trailing void
        template <class MembersA, class MembersB>
        struct padding_aggregate_impl
        {
            template <class ObjectView>
            static auto get_view(ObjectView view) noexcept
                -> decltype(view.template subview<0, 0>())
            {
                return view.template subview<0, 0>();
            }
        };
        template <class HeadA, class... TailA, class HeadB, class... TailB>
        struct padding_aggregate_impl<member_list<HeadA, TailA...>, member_list<HeadB, TailB...>>
        {
            using between = padding_between<HeadA, HeadB>;
            using tail    = padding_aggregate_impl<member_list<TailA...>, member_list<TailB...>>;

            template <class ObjectView,
                      typename = typename std::enable_if<between::has_padding, ObjectView>::type>
            static auto get_view(ObjectView view) noexcept
                -> joined_bit_view<decltype(between::get_subview(view)),
                                   decltype(tail::get_view(view))>
            {
                return join_bit_views(between::get_subview(view), tail::get_view(view));
            }
            template <class ObjectView,
                      typename = typename std::enable_if<!between::has_padding, ObjectView>::type>
            static auto get_view(ObjectView view) noexcept -> decltype(tail::get_view(view))
            {
                return tail::get_view(view);
            }
        };

        template <typename Head, typename... Tail>
        struct padding_aggregate
        : padding_aggregate_impl<member_list<Head, Tail...>, member_list<Tail..., void>>
        {
            using object_type = typename Head::object_type;
        };
    } // namespace detail

    /// Implements the [tiny::padding_traits]() for an aggregate type.
    ///
    /// Simply specialize the traits for your type and inherit from it,
    /// passing `FOONATHAN_TINY_MEMBER(Type, Member)` for every member.
    /// This will automatically calculate the padding bytes and provide a view to it.
    template <class... Members>
    class padding_traits_aggregate
    {
        static_assert(sizeof...(Members) > 0, "type must not be empty");

        using impl        = detail::padding_aggregate<Members...>;
        using object_type = typename impl::object_type;

        static bit_view<unsigned char[sizeof(object_type)], 0, last_bit> byte_view(
            object_type& object) noexcept
        {
            return bit_view<unsigned char[sizeof(object_type)], 0, last_bit>(
                reinterpret_cast<unsigned char*>(&object));
        }
        static bit_view<const unsigned char[sizeof(object_type)], 0, last_bit> byte_view(
            const object_type& object) noexcept
        {
            return bit_view<const unsigned char[sizeof(object_type)], 0, last_bit>(
                reinterpret_cast<const unsigned char*>(&object));
        }

    public:
        static constexpr auto is_specialized = true;

        static auto padding_view(object_type& object) noexcept
            -> decltype(impl::get_view(byte_view(object)))
        {
            return impl::get_view(byte_view(object));
        }

        static auto padding_view(const object_type& object) noexcept
            -> decltype(impl::get_view(byte_view(object)))
        {
            return impl::get_view(byte_view(object));
        }
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_PADDING_TRAITS_HPP_INCLUDED
