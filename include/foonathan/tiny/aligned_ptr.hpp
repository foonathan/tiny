// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_ALIGNED_PTR_HPP_INCLUDED
#define FOONATHAN_TINY_ALIGNED_PTR_HPP_INCLUDED

#include <foonathan/tiny/spare_bits.hpp>

namespace foonathan
{
namespace tiny
{
    /// A pointer to `T` that promises a certain alignment.
    ///
    /// This alignment information is then used for the [tiny::spare_bits_traits]().
    /// This is especially useful for a `void*` which does not have any alignment information.
    ///
    /// \notes Don't bother using this class directly, use the nicer alias [tiny::aligned_ptr]()
    /// instead.
    template <typename T, std::size_t Alignment>
    class basic_aligned_ptr
    {
    public:
        using element_type = T;

        //=== constructors ===//
        /// \effects Initializes the pointer to `nullptr`.
        /// \group nullptr
        basic_aligned_ptr(std::nullptr_t) : pointer_(nullptr) {}
        /// \group nullptr
        basic_aligned_ptr() : pointer_(nullptr) {}

        /// \effects Initializes the pointer to the given value.
        /// \requires The pointer is aligned to the promised alignment.
        explicit basic_aligned_ptr(T* p) noexcept : pointer_(p)
        {
            verify_alignment();
        }

        //=== pointer operators ===//
        /// \returns Whether or not the pointer is `nullptr`.
        explicit operator bool() const noexcept
        {
            return pointer_ != nullptr;
        }

        /// \effects Dereferences the pointer.
        /// \requires The pointer must not be `nullptr`.
        /// \group deref
        template <typename U = element_type>
        U& operator*() const noexcept
        {
            DEBUG_ASSERT(pointer_, detail::precondition_handler{});
            return *pointer_;
        }
        /// \group deref
        element_type* operator->() const noexcept
        {
            DEBUG_ASSERT(pointer_, detail::precondition_handler{});
            return pointer_;
        }

        //=== access ===//
        /// \returns The pointer.
        T* get() const noexcept
        {
            return pointer_;
        }

        /// \returns The promised alignment.
        static constexpr std::size_t alignment() noexcept
        {
            return Alignment;
        }

    private:
        explicit basic_aligned_ptr(std::uintptr_t integer) : pointer_(reinterpret_cast<T*>(integer))
        {}

        void verify_alignment() const noexcept
        {
            auto int_value = reinterpret_cast<std::uintmax_t>(pointer_);
            DEBUG_ASSERT((int_value & (Alignment - 1)) == 0u, detail::precondition_handler{},
                         "pointer not aligned as promised");
        }

        T* pointer_;

        friend struct spare_bits_traits<basic_aligned_ptr<T, Alignment>>;
    };

    /// \returns Whether or not the two pointers are (not) equal.
    /// \group comparison Comparison
    template <typename T, std::size_t Alignment, typename U, std::size_t Alignment2,
              typename = decltype(static_cast<T*>(nullptr) == static_cast<U*>(nullptr))>
    bool operator==(basic_aligned_ptr<T, Alignment>  lhs,
                    basic_aligned_ptr<U, Alignment2> rhs) noexcept
    {
        return lhs.get() == rhs.get();
    }
    /// \group comparison
    template <typename T, std::size_t Alignment, typename U,
              typename = decltype(static_cast<T*>(nullptr) == static_cast<U*>(nullptr))>
    bool operator==(basic_aligned_ptr<T, Alignment> lhs, U* rhs) noexcept
    {
        return lhs.get() == rhs;
    }
    /// \group comparison
    template <typename T, typename U, std::size_t Alignment,
              typename = decltype(static_cast<T*>(nullptr) == static_cast<U*>(nullptr))>
    bool operator==(T* lhs, basic_aligned_ptr<U, Alignment> rhs) noexcept
    {
        return lhs == rhs.get();
    }
    /// \group comparison
    template <typename T, std::size_t Alignment>
    bool operator==(basic_aligned_ptr<T, Alignment> lhs, std::nullptr_t) noexcept
    {
        return lhs.get() == nullptr;
    }
    /// \group comparison
    template <typename U, std::size_t Alignment>
    bool operator==(std::nullptr_t, basic_aligned_ptr<U, Alignment> rhs) noexcept
    {
        return nullptr == rhs.get();
    }

    /// \group comparison
    template <typename T, std::size_t Alignment, typename U, std::size_t Alignment2,
              typename = decltype(static_cast<T*>(nullptr) == static_cast<U*>(nullptr))>
    bool operator!=(basic_aligned_ptr<T, Alignment>  lhs,
                    basic_aligned_ptr<U, Alignment2> rhs) noexcept
    {
        return !(lhs == rhs);
    }
    /// \group comparison
    template <typename T, std::size_t Alignment, typename U,
              typename = decltype(static_cast<T*>(nullptr) == static_cast<U*>(nullptr))>
    bool operator!=(basic_aligned_ptr<T, Alignment> lhs, U* rhs) noexcept
    {
        return !(lhs == rhs);
    }
    /// \group comparison
    template <typename T, typename U, std::size_t Alignment,
              typename = decltype(static_cast<T*>(nullptr) == static_cast<U*>(nullptr))>
    bool operator!=(T* lhs, basic_aligned_ptr<U, Alignment> rhs) noexcept
    {
        return !(lhs == rhs);
    }
    /// \group comparison
    template <typename T, std::size_t Alignment>
    bool operator!=(basic_aligned_ptr<T, Alignment> lhs, std::nullptr_t) noexcept
    {
        return lhs.get() != nullptr;
    }
    /// \group comparison
    template <typename U, std::size_t Alignment>
    bool operator!=(std::nullptr_t, basic_aligned_ptr<U, Alignment> rhs) noexcept
    {
        return nullptr != rhs.get();
    }

    /// Exposes the new spare bits of the aligned pointer.
    template <typename T, std::size_t Alignment>
    struct spare_bits_traits<basic_aligned_ptr<T, Alignment>>
    {
    private:
        using spare_bits_view = bit_view<std::uintptr_t, 0, detail::ilog2_ceil(Alignment)>;

    public:
        static constexpr auto spare_bits = spare_bits_view::size();

        static void clear(basic_aligned_ptr<T, Alignment>& object) noexcept
        {
            put(object, 0u);
        }

        static std::uintmax_t extract(basic_aligned_ptr<T, Alignment> object) noexcept
        {
            auto int_value = reinterpret_cast<std::uintptr_t>(object.get());
            return spare_bits_view(int_value).extract();
        }

        static void put(basic_aligned_ptr<T, Alignment>& object, std::uintmax_t bits) noexcept
        {
            auto int_value = reinterpret_cast<std::uintptr_t>(object.get());
            spare_bits_view(int_value).put(bits);
            object = basic_aligned_ptr<T, Alignment>(int_value);
        }
    };

    /// Tag type that specifies that an object of type `T` will be aligned for `Alignment`.
    ///
    /// It is used for [tiny::aligned_ptr]() and [tiny::pointer_variant_impl]().
    template <typename T, std::size_t Alignment>
    struct aligned_obj
    {
        using type                             = T;
        static constexpr std::size_t alignment = Alignment;
    };

    namespace detail
    {
        template <typename T, std::size_t Alignment>
        struct select_aligned_ptr
        {
            using type = basic_aligned_ptr<T, Alignment>;
        };

        template <typename T, std::size_t Alignment, std::size_t Ignore>
        struct select_aligned_ptr<aligned_obj<T, Alignment>, Ignore>
        {
            using type = basic_aligned_ptr<T, Alignment>;
        };
    } // namespace detail
    /// Convenience alias for [tiny::basic_aligned_ptr]().
    ///
    /// If the type is [tiny::aligned_obj]() it will ignore the second argument and use its
    /// alignment. Otherwise, it will use the alignment of the second argument.
    template <typename T, std::size_t Alignment = alignof(T)>
    using aligned_ptr = typename detail::select_aligned_ptr<T, Alignment>::type;

    /// The alignment of an object of the given type.
    ///
    /// If the type is [tiny::aligned_obj](), the alignment is the alignment specified there.
    /// Otherwise it is `alignof(T)`.
    template <typename T>
    constexpr std::size_t alignment_of()
    {
        return aligned_ptr<T>::alignment();
    }
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_ALIGNED_PTR_HPP_INCLUDED
