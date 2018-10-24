// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_POINTER_VARIANT_IMPL_HPP_INCLUDED
#define FOONATHAN_TINY_POINTER_VARIANT_IMPL_HPP_INCLUDED

#include <foonathan/tiny/pointer_tiny_storage.hpp>
#include <foonathan/tiny/tiny_int.hpp>

namespace foonathan
{
namespace tiny
{
    namespace detail
    {
        //=== variadic min ===//
        template <typename T>
        constexpr T min(T a)
        {
            return a;
        }
        template <typename T>
        constexpr T min(T a, T b)
        {
            return a < b ? a : b;
        }
        template <typename Head, typename... Tail>
        constexpr Head min(Head h, Tail... tail)
        {
            return min(h, min(tail...));
        }

        //=== storage type ===//
        template <typename... Ts>
        using pointer_variant_value_type = aligned_obj<const void, min(alignment_of<Ts>()...)>;

        template <typename... Ts>
        using pointer_variant_tag = tiny_unsigned<detail::ilog2_ceil(sizeof...(Ts)), std::size_t>;

        template <typename... Ts>
        using pointer_variant_storage
            = pointer_tiny_storage<pointer_variant_value_type<Ts...>, pointer_variant_tag<Ts...>>;

        //=== variant tag calculation ===//
        template <typename T, typename... Ts>
        struct get_pointer_tag;

        template <typename T>
        struct get_pointer_tag<T> : std::integral_constant<std::size_t, 0>
        {
            using is_valid = std::false_type;
        };

        template <typename T, typename... Tail>
        struct get_pointer_tag<T, T, Tail...> : std::integral_constant<std::size_t, 0>
        {
            using is_valid = std::true_type;
        };
        template <typename T, std::size_t Alignment, typename... Tail>
        struct get_pointer_tag<T, aligned_obj<T, Alignment>, Tail...>
        : std::integral_constant<std::size_t, 0>
        {
            using is_valid = std::true_type;
        };

        template <typename T, typename Head, typename... Tail>
        struct get_pointer_tag<T, Head, Tail...>
        : std::integral_constant<std::size_t, 1 + get_pointer_tag<T, Tail...>::value>
        {
            using is_valid = typename get_pointer_tag<T, Tail...>::is_valid;
        };
    } // namespace detail

    /// The storage implementation of a variant of pointer to one of the types that uses spare bits
    /// whenever possible.
    ///
    /// It will store a `const void*` and information about the current type.
    /// This is done in the spare bits of the pointer if there are enough.
    /// The amount of spare bits is dependent on the minimal alignment of all types,
    /// respecting any [tiny::aligned_obj]() to override the alignment.
    ///
    /// It is just a low-level implementation helper for an actual variant.
    /// A proper variant type should be built on top of it.
    template <typename... Ts>
    class pointer_variant_impl
    {
        using storage_type = detail::pointer_variant_storage<Ts...>;

    public:
        using is_compressed = typename storage_type::is_compressed;

        /// The tag of this element type.
        ///
        /// `::is_valid` is [std::true_type]() if a pointer to the element type can be stored,
        /// [std::false_type]() otherwise.
        /// `::value` returns the index of the element type in the list of pointers,
        /// if the element type is valid.
        template <typename T>
        using tag_of = detail::get_pointer_tag<T, Ts...>;

        //=== constructors ===//
        /// \effects Same as `reset()`.
        /// \group ctor
        explicit pointer_variant_impl(std::nullptr_t)
        {
            reset(nullptr);
        }
        /// \group ctor
        template <typename T>
        explicit pointer_variant_impl(T* ptr)
        {
            reset(ptr);
        }

        //=== modifiers ===//
        /// \effects Resets the variant to `nullptr`.
        void reset(std::nullptr_t) noexcept
        {
            storage_.pointer() = nullptr;
        }

        /// \effects Resets the variant to a pointer to the given object.
        /// \notes The pointer may be `nullptr`.
        template <typename T>
        void reset(T* ptr) noexcept
        {
            static_assert(typename tag_of<T>::is_valid{}, "type cannot be stored in variant");

            storage_.pointer() = ptr;
            storage_.tiny()    = tag_of<T>::value;
        }

        //=== accessors ===//
        /// \returns Whether or not the variant currently points to an object,
        /// i.e. it is not `nullptr`.
        bool has_value() const noexcept
        {
            return storage_.pointer() != nullptr;
        }

        /// \returns The tag value of the currently active element type,
        /// or an invalid tag value if there is none.
        std::size_t tag() const noexcept
        {
            if (has_value())
                return storage_.tiny();
            else
                return std::size_t(-1);
        }

        /// \returns The untyped pointer.
        const void* get() const noexcept
        {
            return storage_.pointer();
        }

        /// \returns The typed pointer to `T`.
        /// \requires It must contain a non-null pointer to the given type.
        template <typename T>
        T* pointer_to() const noexcept
        {
            using tag_of_t = tag_of<T>;
            static_assert(tag_of_t::is_valid::value, "type cannot be stored in variant");
            DEBUG_ASSERT(tag() == tag_of_t::value, detail::precondition_handler{});
            // C style cast because we might need to cast away const-ness
            return (T*)(get());
        }

    private:
        storage_type storage_;
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_POINTER_VARIANT_IMPL_HPP_INCLUDED
