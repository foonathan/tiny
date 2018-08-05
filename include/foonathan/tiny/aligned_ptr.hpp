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
        /// A pointer type that promises a certain alignment.
        ///
        /// This alignment information is then used for the [tiny::spare_bits_traits]().
        /// This is especially useful for a `void*` which does not have any alignment information.
        template <typename Pointer, std::size_t Alignment>
        class aligned_ptr
        {
            static_assert(std::is_pointer<Pointer>::value, "not a pointer type");

        public:
            using element_type = typename std::remove_pointer<Pointer>::type;

            //=== constructors ===//
            /// \effects Initializes the pointer to `nullptr`.
            /// \group nullptr
            aligned_ptr(std::nullptr_t) : pointer_(nullptr) {}
            /// \group nullptr
            aligned_ptr() : pointer_(nullptr) {}

            /// \effects Initializes the pointer to the given value.
            /// \requires The pointer is aligned to the promised alignment.
            explicit aligned_ptr(Pointer p) noexcept : pointer_(p)
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
            template <typename T = element_type>
            T& operator*() const noexcept
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
            Pointer get() const noexcept
            {
                return pointer_;
            }

            /// \returns The promised alignment.
            static constexpr std::size_t alignment() noexcept
            {
                return Alignment;
            }

        private:
            explicit aligned_ptr(std::uintptr_t integer)
            : pointer_(reinterpret_cast<Pointer>(integer))
            {
            }

            void verify_alignment() const noexcept
            {
                auto int_value = reinterpret_cast<std::uintmax_t>(pointer_);
                DEBUG_ASSERT((int_value & (Alignment - 1)) == 0u, detail::precondition_handler{},
                             "pointer not aligned as promised");
            }

            Pointer pointer_;

            friend struct spare_bits_traits<aligned_ptr<Pointer, Alignment>>;
        };

        /// \returns Whether or not the two pointers are (not) equal.
        /// \group comparison Comparison
        template <typename Pointer, std::size_t Alignment, typename Pointer2,
                  std::size_t Alignment2, typename = decltype(Pointer() == Pointer2())>
        bool operator==(aligned_ptr<Pointer, Alignment>   lhs,
                        aligned_ptr<Pointer2, Alignment2> rhs) noexcept
        {
            return lhs.get() == rhs.get();
        }
        /// \group comparison
        template <typename Pointer, std::size_t Alignment, typename Pointer2,
                  typename = decltype(Pointer() == Pointer2())>
        bool operator==(aligned_ptr<Pointer, Alignment> lhs, Pointer2 rhs) noexcept
        {
            return lhs.get() == rhs;
        }
        /// \group comparison
        template <typename Pointer, typename Pointer2, std::size_t Alignment2,
                  typename = decltype(Pointer() == Pointer2())>
        bool operator==(Pointer lhs, aligned_ptr<Pointer2, Alignment2> rhs) noexcept
        {
            return lhs == rhs.get();
        }

        /// \group comparison
        template <typename Pointer, std::size_t Alignment, typename Pointer2,
                  std::size_t Alignment2, typename = decltype(Pointer() == Pointer2())>
        bool operator!=(aligned_ptr<Pointer, Alignment>   lhs,
                        aligned_ptr<Pointer2, Alignment2> rhs) noexcept
        {
            return !(lhs == rhs);
        }
        /// \group comparison
        template <typename Pointer, std::size_t Alignment, typename Pointer2,
                  typename = decltype(Pointer() == Pointer2())>
        bool operator!=(aligned_ptr<Pointer, Alignment> lhs, Pointer2 rhs) noexcept
        {
            return !(lhs == rhs);
        }
        /// \group comparison
        template <typename Pointer, typename Pointer2, std::size_t Alignment2,
                  typename = decltype(Pointer() == Pointer2())>
        bool operator!=(Pointer lhs, aligned_ptr<Pointer2, Alignment2> rhs) noexcept
        {
            return !(lhs == rhs);
        }

        /// Exposes the new spare bits of the aligned pointer.
        template <typename Pointer, std::size_t Alignment>
        struct spare_bits_traits<aligned_ptr<Pointer, Alignment>>
        {
            using spare_bits_view = bit_view<std::uintptr_t, 0, detail::ilog2_ceil(Alignment)>;

        public:
            static constexpr auto spare_bits = spare_bits_view::size();

            static void clear(aligned_ptr<Pointer, Alignment>& object) noexcept
            {
                put(object, 0u);
            }

            static std::uintmax_t extract(aligned_ptr<Pointer, Alignment> object) noexcept
            {
                auto int_value = reinterpret_cast<std::uintptr_t>(object.get());
                return spare_bits_view(int_value).extract();
            }

            static void put(aligned_ptr<Pointer, Alignment>& object, std::uintmax_t bits) noexcept
            {
                auto int_value = reinterpret_cast<std::uintptr_t>(object.get());
                spare_bits_view(int_value).put(bits);
                object = aligned_ptr<Pointer, Alignment>(int_value);
            }
        };
    } // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_ALIGNED_PTR_HPP_INCLUDED
