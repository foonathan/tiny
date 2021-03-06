// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TINY_TYPE_STORAGE_HPP_INCLUDED
#define FOONATHAN_TINY_TINY_TYPE_STORAGE_HPP_INCLUDED

#include <cstring>

#include <foonathan/tiny/detail/index_sequence.hpp>
#include <foonathan/tiny/tiny_type.hpp>

namespace foonathan
{
namespace tiny
{
    namespace tiny_storage_detail
    {
        //=== is_target ===//
        template <std::size_t Index, typename Target, class TinyType>
        struct is_target : std::false_type
        {};

        template <std::size_t Index, typename Target>
        struct is_target<Index, Target, Target> : std::true_type
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
            using type                        = void;
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
            static_assert(result::found_count <= 1, "tiny type ambiguous, use index");

            using type = typename result::type;

            static constexpr std::size_t offset = result::offset;
        };

        template <typename Target, class... TinyTypes>
        using tiny_type = typename find<Target, TinyTypes...>::type;

        template <typename Target, class... TinyTypes>
        static constexpr std::size_t offset_of() noexcept
        {
            return find<Target, TinyTypes...>::offset;
        }

        //=== total_size ===//
        template <class... TinyTypes>
        struct total_size;

        template <>
        struct total_size<> : std::integral_constant<std::size_t, 0>
        {};

        template <class Head, class... Tail>
        struct total_size<Head, Tail...>
        : std::integral_constant<std::size_t, Head::bit_size() + total_size<Tail...>::value>
        {};

    } // namespace tiny_storage_detail

#if 0
    /// The policy controlling how the tiny types are stored.
    ///
    /// It must be able to store all of the tiny types given to it.
    template <class ... TinyTypes>
    class TinyStoragePolicy
    {
        /// Whether or not the tiny types are stored without using extra space.
        using is_compressed = std::integral_constant<bool, …>;

        TinyStoragePolicy() noexcept;

        /// \returns A bit view into the bits used for the storage.
        BitView storage_view() noexcept;
        BitView storage_view() const noexcept;
    };
#endif

    /// \returns The total number of bits required to store all the tiny types.
    template <class... TinyTypes>
    constexpr std::size_t total_bit_size() noexcept
    {
        return tiny_storage_detail::total_size<TinyTypes...>::value;
    }

    /// A wrapper over the `BitView` that allows viewing the bits as the specified types.
    template <typename BitView, class... TinyTypes>
    class basic_tiny_storage_view
    {
        static constexpr auto bit_size_needed = total_bit_size<TinyTypes...>();
        static_assert(bit_size_needed <= BitView::size(), "bit view overflow");

        template <class Type, std::size_t Offset>
        using proxy = decltype(make_tiny_proxy<Type>(
            std::declval<BitView>().template subview<Offset, Offset + Type::bit_size()>()));

        template <typename Tag>
        using proxy_of = proxy<tiny_storage_detail::tiny_type<Tag, TinyTypes...>,
                               tiny_storage_detail::offset_of<Tag, TinyTypes...>()>;
        template <typename Tag>
        proxy_of<Tag> get_impl() const noexcept
        {
            using type            = tiny_storage_detail::tiny_type<Tag, TinyTypes...>;
            constexpr auto offset = tiny_storage_detail::offset_of<Tag, TinyTypes...>();

            return make_tiny_proxy<type>(
                view_.template subview<offset, offset + type::bit_size()>());
        }

    public:
        //=== constructors ====//
        /// Creates it from the given bit view.
        basic_tiny_storage_view(BitView view) noexcept : view_(view) {}

        //=== access ===//
        /// Array access operator.
        /// \returns The proxy for the tiny type `T`.
        /// It is an error if there is more than one of those tiny types in the storage.
        template <class T>
        auto operator[](T) const noexcept -> proxy_of<T>
        {
            return get_impl<T>();
        }

        /// \returns The proxy of the tiny type at the specified index.
        template <std::size_t I>
        auto at() const noexcept -> proxy_of<std::integral_constant<std::size_t, I>>
        {
            static_assert(I < sizeof...(TinyTypes), "index out of bounds");
            return get_impl<std::integral_constant<std::size_t, I>>();
        }

        /// Convenience access for a single tiny type.
        /// \returns `at<0>()`.
        /// \requires Only one tiny type must be stored.
        template <std::size_t Dummy = sizeof...(TinyTypes)>
        auto tiny() const noexcept -> proxy_of<std::integral_constant<std::size_t, Dummy - 1>>
        {
            static_assert(Dummy == 1, "only allowed for 1 tiny type");
            return at<0>();
        }

        /// \returns A [tiny::bit_view]() of the views that are not used but there for padding.
        auto spare_bits() const noexcept
            -> decltype(std::declval<BitView>().template subview<bit_size_needed, last_bit>())
        {
            return view_.template subview<bit_size_needed, last_bit>();
        }

    private:
        BitView view_;
    };

    /// A type that has at least `Bits` bits and is thus able to store tiny types.
    template <std::size_t Bits>
    using tiny_storage_type
        = unsigned char[Bits == 0 ? 1 : Bits / CHAR_BIT + (Bits % CHAR_BIT == 0 ? 0 : 1)];

    /// A type that is able to store the specified tiny types.
    template <class... TinyTypes>
    using tiny_storage_type_for = tiny_storage_type<total_bit_size<TinyTypes...>()>;

    /// The basic template for storing multiple tiny types.
    ///
    /// It provides and implements the accessing function and manages the exact offsets of the tiny
    /// types. An implementation provides the `TinyStoragePolicy`, which is stored as a base class,
    /// and provides the actual storage for the types.
    ///
    /// Then the implementation inherits from this class providing additional member functions as
    /// needed.
    template <class TinyStoragePolicy, class... TinyTypes>
    class basic_tiny_storage : TinyStoragePolicy
    {
        using view
            = basic_tiny_storage_view<decltype(std::declval<TinyStoragePolicy&>().storage_view()),
                                      TinyTypes...>;
        using cview = basic_tiny_storage_view<
            decltype(std::declval<const TinyStoragePolicy&>().storage_view()), TinyTypes...>;

        struct default_ctor_tag
        {};

    public:
        /// Whether or not the tiny types are stored without using extra space.
        using is_compressed = typename TinyStoragePolicy::is_compressed;

        //=== constructors ===//
        /// Default constructor.
        /// \effects Initializes all tiny types to the value corresponding to all zeroes.
        /// \param 1
        /// \exclude
        template <typename T = default_ctor_tag>
        basic_tiny_storage(typename std::enable_if<sizeof...(TinyTypes) != 0, T>::type
                           = {}) noexcept
        {
            clear_bits(this->storage_view());
        }

        /// Object constructor.amazon
        /// \effects Initializes all tiny types from the corresponding object type.
        basic_tiny_storage(typename TinyTypes::object_type... objects) noexcept
        : basic_tiny_storage(detail::make_index_sequence<sizeof...(TinyTypes)>{}, objects...)
        {}

        basic_tiny_storage(basic_tiny_storage&&)      = default;
        basic_tiny_storage(const basic_tiny_storage&) = default;

        basic_tiny_storage& operator=(const basic_tiny_storage&) = default;
        basic_tiny_storage& operator=(basic_tiny_storage&&) = default;

        //=== access ===//
        /// Array access operator.
        /// \returns The proxy for the tiny type `T`.
        /// It is an error if there is more than one of those tiny types in the storage.
        /// \group array
        template <class T>
        auto operator[](T) noexcept -> decltype(std::declval<view>()[T{}])
        {
            return view(this->storage_view())[T{}];
        }
        /// \group array
        template <class T>
        auto operator[](T) const noexcept -> decltype(std::declval<cview>()[T{}])
        {
            return cview(this->storage_view())[T{}];
        }

        /// \returns The proxy of the tiny type at the specified index.
        /// \group at
        template <std::size_t I>
        auto at() noexcept -> decltype(std::declval<view>().template at<I>())
        {
            return view(this->storage_view()).template at<I>();
        }
        /// \group at
        template <std::size_t I>
        auto at() const noexcept -> decltype(std::declval<cview>().template at<I>())
        {
            return cview(this->storage_view()).template at<I>();
        }

        /// Convenience access for a single tiny type.
        /// \returns `at<0>()`.
        /// \requires Only one tiny type must be stored.
        /// \group tiny
        template <typename View = view>
        auto tiny() noexcept -> decltype(std::declval<View>().tiny())
        {
            return view(this->storage_view()).tiny();
        }
        /// \group tiny
        template <typename View = cview>
        auto tiny() const noexcept -> decltype(std::declval<View>().tiny())
        {
            return cview(this->storage_view()).tiny();
        }

        /// \returns A [tiny::bit_view]() of the views that are not used but there for padding.
        /// \group spare_bits
        auto spare_bits() noexcept -> decltype(std::declval<view>().spare_bits())
        {
            return view(storage_policy().storage_view()).spare_bits();
        }
        /// \group spare_bits
        auto spare_bits() const noexcept -> decltype(std::declval<cview>().spare_bits())
        {
            return cview(storage_policy().storage_view()).spare_bits();
        }

    protected:
        ~basic_tiny_storage() noexcept = default;

        /// \returns The storage policy.
        /// \group storage_policy
        TinyStoragePolicy& storage_policy() noexcept
        {
            return *this;
        }
        /// \group storage_policy
        const TinyStoragePolicy& storage_policy() const noexcept
        {
            return *this;
        }

    private:
        template <std::size_t... Indices>
        basic_tiny_storage(detail::index_sequence<Indices...>,
                           typename TinyTypes::object_type... objects)
        {
            bool for_each[] = {(at<Indices>() = objects, true)..., true};
            (void)for_each;
        }
    };

    namespace tiny_storage_detail
    {
        template <class... TinyTypes>
        class embedded_storage_policy
        {
            using is_compressed = std::false_type;

            embedded_storage_policy() noexcept = default;

            using storage_type = tiny_storage_type_for<TinyTypes...>;

            bit_view<storage_type, 0, last_bit> storage_view() noexcept
            {
                return make_bit_view<0, last_bit>(storage_);
            }
            bit_view<const storage_type, 0, last_bit> storage_view() const noexcept
            {
                return make_bit_view<0, last_bit>(storage_);
            }

            storage_type storage_;

            friend basic_tiny_storage<embedded_storage_policy<TinyTypes...>, TinyTypes...>;
        };
    } // namespace tiny_storage_detail

    /// A compressed tuple of tiny types.
    template <class... TinyTypes>
    class tiny_storage
    : public basic_tiny_storage<tiny_storage_detail::embedded_storage_policy<TinyTypes...>,
                                TinyTypes...>
    {
    public:
        using basic_tiny_storage<tiny_storage_detail::embedded_storage_policy<TinyTypes...>,
                                 TinyTypes...>::basic_tiny_storage;
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TINY_TYPE_STORAGE_HPP_INCLUDED
