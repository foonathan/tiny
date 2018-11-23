// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TAGGED_UNION_IMPL_HPP_INCLUDED
#define FOONATHAN_TINY_TAGGED_UNION_IMPL_HPP_INCLUDED

#include <new>

#include <foonathan/tiny/tiny_int.hpp>
#include <foonathan/tiny/tiny_storage.hpp>

namespace foonathan
{
namespace tiny
{
    /// \exclude
    namespace tagged_union_detail
    {
        template <class UnionTypes, std::size_t I, typename... T>
        union types_storage;
    } // namespace tagged_union_detail

    template <class UnionTypes>
    class tagged_union_impl;

    /// A list of types that are being stored in a [tiny::tagged_union_impl]().
    template <class... T>
    struct union_types
    {
        /// \exclude
        using tag = tiny_int_range<0, std::intmax_t(sizeof...(T)), std::size_t>;

        /// \exclude
        using storage = tagged_union_detail::types_storage<union_types<T...>, 0, T...>;
    };

    /// The tag of a [tiny::tagged_union_impl]().
    ///
    /// Every type of the union must have an object of this type as its first member.
    /// It stores the tag as well as some other tiny types that can fit into the same space.
    template <class UnionTypes>
    class tagged_union_tag
    {
        using tiny_tag      = typename UnionTypes::tag;
        using storage_type  = tiny_storage_type_for<tiny_tag>;
        using tag_view      = bit_view<storage_type, 0, tiny_tag::bit_size()>;
        using tag_cview     = bit_view<const storage_type, 0, tiny_tag::bit_size()>;
        using storage_view  = bit_view<storage_type, tiny_tag::bit_size(), last_bit>;
        using storage_cview = bit_view<const storage_type, tiny_tag::bit_size(), last_bit>;

        storage_type storage_;

    public:
        tagged_union_tag() noexcept = default;

        /// The amount of bits that are not filled by the tag.
        static constexpr std::size_t spare_bits = storage_view::size();

        /// \returns A [tiny::basic_tiny_storage_view]() that allows access to the specific tiny
        /// types stored in the spare bits of the tag.
        /// \notes For consistent result the tiny types should always be the same on every
        /// call.
        /// \group tiny_view
        template <typename... TinyTypes>
        auto tiny_view(tiny_types<TinyTypes...>) noexcept
            -> basic_tiny_storage_view<storage_view, TinyTypes...>
        {
            return storage_view(storage_);
        }
        /// \group tiny_view
        template <typename... TinyTypes>
        auto tiny_view(tiny_types<TinyTypes...>) const noexcept
            -> basic_tiny_storage_view<storage_cview, TinyTypes...>
        {
            return storage_cview(storage_);
        }

        /// \returns A [tiny::basic_tiny_storage_view]() that uses the spare bits of the tag and
        /// additional storage.
        /// \notes For consistent result the tiny types should always be the same on every
        /// call.
        /// \group tiny_view_storage
        template <typename... TinyTypes, class BitView>
        auto tiny_view(tiny_types<TinyTypes...>, BitView additional_storage) noexcept
            -> basic_tiny_storage_view<joined_bit_view<storage_view, BitView>, TinyTypes...>
        {
            return join_bit_views(storage_view(storage_), additional_storage);
        }
        /// \group tiny_view_storage
        template <typename... TinyTypes, class BitView>
        auto tiny_view(tiny_types<TinyTypes...>, BitView additional_storage) const noexcept
            -> basic_tiny_storage_view<joined_bit_view<storage_cview, BitView>, TinyTypes...>
        {
            return join_bit_views(storage_cview(storage_), additional_storage);
        }

    private:
        void set_tag(std::size_t tag) noexcept
        {
            make_tiny_proxy<tiny_tag>(tag_view(storage_)) = tag;
        }
        std::size_t get_tag() const noexcept
        {
            return make_tiny_proxy<tiny_tag>(tag_cview(storage_));
        }

        template <class, std::size_t, typename...>
        friend union tagged_union_detail::types_storage;
        template <class>
        friend class tagged_union_impl;
    };

    /// \exclude
    namespace tagged_union_detail
    {
        template <typename T>
        struct type_tag
        {};

        template <class UnionTypes, std::size_t I, typename... T>
        union types_storage;

        template <class UnionTypes, std::size_t I, typename Head, typename... Tail>
        union types_storage<UnionTypes, I, Head, Tail...>
        {
            static_assert(std::is_standard_layout<Head>::value,
                          "all types of the tagged union must be standard layout");

            tagged_union_tag<UnionTypes>              tag;
            Head                                      head;
            types_storage<UnionTypes, I + 1, Tail...> tail;

            types_storage() {}

            template <typename... Args>
            void construct(type_tag<Head>, Args&&... args)
            {
                ::new (static_cast<void*>(&head)) Head(static_cast<Args&&>(args)...);
                tag.set_tag(I);
            }
            template <typename T, typename... Args>
            void construct(type_tag<T> tag, Args&&... args)
            {
                tail.construct(tag, static_cast<Args&&>(args)...);
            }

            bool has_value(type_tag<Head>) const noexcept
            {
                return tag.get_tag() == I;
            }
            template <typename T>
            bool has_value(type_tag<T> tag) const noexcept
            {
                return tail.has_value(tag);
            }

            Head& get(type_tag<Head>) noexcept
            {
                return head;
            }
            const Head& get(type_tag<Head>) const noexcept
            {
                return head;
            }
            template <typename T>
            T& get(type_tag<T> tag) noexcept
            {
                return tail.get(tag);
            }
            template <typename T>
            const T& get(type_tag<T> tag) const noexcept
            {
                return tail.get(tag);
            }
        };

        template <class UnionTypes, std::size_t I>
        union types_storage<UnionTypes, I>
        {
            tagged_union_tag<UnionTypes> tag;

            types_storage() = default;

            template <typename T, typename... Args>
            void construct(type_tag<T>, Args&&...)
            {
                static_assert(sizeof(T) != sizeof(T), "type not stored in tagged_union");
            }

            template <typename T>
            bool has_value(type_tag<T>) const noexcept
            {
                return false;
            }

            template <typename T>
            T& get(type_tag<T>) noexcept
            {
                static_assert(sizeof(T) != sizeof(T), "type not stored in tagged_union");
                return DEBUG_UNREACHABLE(detail::assert_handler{});
            }
            template <typename T>
            const T& get(type_tag<T>) const noexcept
            {
                static_assert(sizeof(T) != sizeof(T), "type not stored in tagged_union");
                return DEBUG_UNREACHABLE(detail::assert_handler{});
            }
        };

        template <class UnionTypes>
        using types_storage_for = typename UnionTypes::storage;
    } // namespace tagged_union_detail

    /// An intrusive tagged union implementation helper.
    ///
    /// It is just a low-level implementation helper.
    /// A proper union type should be built on top of it.
    ///
    /// The union can either store an object of type `T` or is invalid and stores nothing.
    /// If it is invalid, only a new value can be put inside of it; it can't answer whether it
    /// stores an object of type `T`. This is done to prevent using an additional "empty union" tag
    /// value.
    ///
    /// If you want to implement a union that can be empty, use [tiny::tagged_union_empty]() as
    /// dummy type.
    ///
    /// \requires Every type of the tagged union must be a standard layout type and have the
    /// [tiny::tagged_union_tag]() as first member.
    template <class UnionTypes>
    class tagged_union_impl
    {
    public:
        using value_types = UnionTypes;

        //=== constructors ===//
        /// \effects Creates an invalid union.
        tagged_union_impl() = default;

        /// \effects Does nothing.
        /// \notes This will leak the currently stored value.
        ~tagged_union_impl() = default;

        tagged_union_impl(const tagged_union_impl&) = delete;

        tagged_union_impl& operator=(const tagged_union_impl&) = delete;

        //=== mutators ===//
        /// \effects Creates a value of the specified type by forwarding the arguments.
        /// \requires `T` is a type that can be stored and the union is currently in the invalid
        /// state.
        template <typename T, typename... Args>
        void create_value(Args&&... args)
        {
            storage_.construct(tagged_union_detail::type_tag<T>{}, static_cast<Args&&>(args)...);
        }

        /// \effects Destroys the currently stored value of the specified type
        /// and puts it into the invalid state.
        /// \requires `has_value<T>() == true`.
        template <typename T>
        void destroy_value() noexcept
        {
            value<T>().~T();
        }

        //=== accessors ===//
        /// \returns The tag of the object currently stored.
        /// \requires The union is not in the invalid state.
        std::size_t tag() const noexcept
        {
            return storage_.tag.get_tag();
        }

        /// \returns Whether or not the union currently stores a value of the specified type.
        /// \requires The union is not in the invalid state.
        template <typename T>
        bool has_value() const noexcept
        {
            return storage_.has_value(tagged_union_detail::type_tag<T>{});
        }

        /// \returns A reference to the value of the specified type that is currently being stored.
        /// \requires `has_value<T>() == true`.
        /// \group value
        template <typename T>
        T& value() noexcept
        {
            DEBUG_ASSERT(has_value<T>(), detail::precondition_handler{});
            return storage_.get(tagged_union_detail::type_tag<T>{});
        }

        /// \group value
        template <typename T>
        const T& value() const noexcept
        {
            DEBUG_ASSERT(has_value<T>(), detail::precondition_handler{});
            return storage_.get(tagged_union_detail::type_tag<T>{});
        }

    private:
        tagged_union_detail::types_storage_for<UnionTypes> storage_;
    };

    /// Dummy type to allow an empty [tiny::tagged_union_impl]().
    template <class UnionTypes>
    struct tagged_union_empty
    {
        tagged_union_tag<UnionTypes> tag;
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TAGGED_UNION_IMPL_HPP_INCLUDED
