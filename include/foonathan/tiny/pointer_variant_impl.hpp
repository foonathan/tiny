// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_POINTER_VARIANT_IMPL_HPP_INCLUDED
#define FOONATHAN_TINY_POINTER_VARIANT_IMPL_HPP_INCLUDED

#include <foonathan/tiny/aligned_ptr.hpp>
#include <foonathan/tiny/spare_bits.hpp>

namespace foonathan
{
namespace tiny
{
    namespace detail
    {
        template <bool IsCompressed, typename... Ts>
        struct spare_bits_traits_pointer_variant_impl;

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
        using pointer_variant_storage = aligned_ptr<const void, min(alignment_of<Ts>()...)>;

        //=== variant tag storage ===//
        template <bool IsCompressed>
        struct pointer_variant_tag_impl;

        template <>
        struct pointer_variant_tag_impl<true>
        {
        protected:
            using is_compressed = std::true_type;

            template <typename AlignedPtr>
            std::size_t get_tag(AlignedPtr ptr) const
            {
                return extract_spare_bits(ptr);
            }

            template <typename AlignedPtr>
            void set_tag(AlignedPtr& ptr, std::size_t tag)
            {
                put_spare_bits(ptr, tag);
            }

            template <typename AlignedPtr>
            AlignedPtr extract_ptr(const AlignedPtr& ptr) const
            {
                return extract_object(ptr);
            }
        };

        template <>
        struct pointer_variant_tag_impl<false>
        {
        protected:
            using is_compressed = std::false_type;

            template <typename AlignedPtr>
            std::size_t get_tag(AlignedPtr) const
            {
                return tag_;
            }

            template <typename AlignedPtr>
            void set_tag(AlignedPtr&, std::size_t tag)
            {
                tag_ = tag;
            }

            template <typename AlignedPtr>
            AlignedPtr extract_ptr(const AlignedPtr& ptr) const
            {
                return ptr;
            }

        private:
            std::size_t tag_;

            template <bool, typename...>
            friend struct spare_bits_traits_pointer_variant_impl;
        };

        template <typename... Ts>
        using pointer_variant_tag
            = pointer_variant_tag_impl<(1u << spare_bits<detail::pointer_variant_storage<Ts...>>())
                                       >= sizeof...(Ts)>;

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
    class pointer_variant_impl : detail::pointer_variant_tag<Ts...>
    {
    public:
        using is_compressed = typename detail::pointer_variant_tag<Ts...>::is_compressed;

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
            ptr_ = nullptr;
        }

        /// \effects Resets the variant to a pointer to the given object.
        /// \notes The pointer may be `nullptr`.
        template <typename T>
        void reset(T* ptr) noexcept
        {
            static_assert(typename tag_of<T>::is_valid{}, "type cannot be stored in variant");

            ptr_ = detail::pointer_variant_storage<Ts...>(ptr);
            this->set_tag(ptr_, tag_of<T>::value);
        }

        //=== accessors ===//
        /// \returns Whether or not the variant currently points to an object,
        /// i.e. it is not `nullptr`.
        bool has_value() const noexcept
        {
            return extract_object(ptr_) != nullptr;
        }

        /// \returns The tag value of the currently active element type,
        /// or an invalid tag value if there is none.
        std::size_t tag() const noexcept
        {
            if (has_value())
                return this->get_tag(ptr_);
            else
                return std::size_t(-1);
        }

        /// \returns The untyped pointer.
        const void* get() const noexcept
        {
            return extract_object(ptr_).get();
        }

        /// \returns The typed pointer to `T`.
        /// \requires It must contain a pointer to the given type.
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
        detail::pointer_variant_storage<Ts...> ptr_;

        friend detail::spare_bits_traits_pointer_variant_impl<is_compressed::value, Ts...>;
    };

    namespace detail
    {
        template <typename... Ts>
        struct spare_bits_traits_pointer_variant_impl<true, Ts...>
        {
        private:
            using impl_traits     = spare_bits_traits<detail::pointer_variant_storage<Ts...>>;
            using spare_bits_view = bit_view<std::uintmax_t, detail::ilog2_ceil(sizeof...(Ts)),
                                             impl_traits::spare_bits>;

        public:
            static constexpr auto spare_bits = spare_bits_view::size();

            static void clear(pointer_variant_impl<Ts...>& object) noexcept
            {
                impl_traits::clear(object.ptr_);
            }

            static std::uintmax_t extract(pointer_variant_impl<Ts...> object) noexcept
            {
                return impl_traits::extract(object.ptr_);
            }

            static void put(pointer_variant_impl<Ts...>& object, std::uintmax_t bits) noexcept
            {
                impl_traits::put(object.ptr_, bits);
            }
        };

        template <typename... Ts>
        struct spare_bits_traits_pointer_variant_impl<false, Ts...>
        {
        private:
            using spare_bits_view = bit_view<std::size_t, detail::ilog2_ceil(sizeof...(Ts)),
                                             sizeof(std::size_t) * CHAR_BIT>;

        public:
            static constexpr auto spare_bits = spare_bits_view::size();

            static void clear(pointer_variant_impl<Ts...>& object) noexcept
            {
                put(object, 0);
            }

            static std::uintmax_t extract(pointer_variant_impl<Ts...> object) noexcept
            {
                return spare_bits_view(object.tag_).extract();
            }

            static void put(pointer_variant_impl<Ts...>& object, std::uintmax_t bits) noexcept
            {
                spare_bits_view(object.tag_).put(bits);
            }
        };
    } // namespace detail

    /// Exposes additional spare bits of the [tiny::pointer_variant_impl]().
    template <typename... Ts>
    struct spare_bits_traits<pointer_variant_impl<Ts...>>
    : detail::spare_bits_traits_pointer_variant_impl<
          pointer_variant_impl<Ts...>::is_compressed::value, Ts...>
    {};
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_POINTER_VARIANT_IMPL_HPP_INCLUDED
