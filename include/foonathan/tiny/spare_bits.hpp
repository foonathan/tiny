// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_SPARE_BITS_HPP_INCLUDED
#define FOONATHAN_TINY_SPARE_BITS_HPP_INCLUDED

#include <foonathan/tiny/detail/ilog2.hpp>
#include <foonathan/tiny/bit_view.hpp>

namespace foonathan
{
    namespace tiny
    {
        //=== spare bits traits implementations ===//
        /// The default implementation of the spare bits, i.e. no spare bits at all.
        template <typename T>
        struct spare_bits_traits_default
        {
            /// The number of spare bits in the object.
            static constexpr std::size_t spare_bits = 0u;

            /// \effects Puts the object into a valid state by clearing all spare bits, if necessary.
            ///
            /// Member functions are only invoked when the object is in this state.
            /// The only exception is the copy/move constructor/assignment:
            /// they can be called on a `*this` containing arbitrary values for the spare bits.
            /// But the `rhs` will be in a cleared state.
            static void clear(T& obj) noexcept
            {
                (void)obj;
            }

            /// \returns The spare bits currently stored in the object.
            static std::uintmax_t extract(const T& obj) noexcept
            {
                (void)obj;
                return std::uintmax_t(0);
            }

            /// \effects Inserts the spare bits into the object.
            static void put(T& obj, std::uintmax_t spare_bits) noexcept
            {
                (void)obj;
                (void)spare_bits;
            }
        };

        /// The spare bits of a boolean.
        ///
        /// It works for any type that behaves like `bool` where `false` is stored as integer `0` and `true` as `1`.
        template <typename Boolean>
        class spare_bits_traits_boolean
        {
            static_assert(sizeof(Boolean) == 1, "boolean must be one byte");
            static_assert(std::is_constructible<Boolean, bool>::value,
                          "must be constructible from bool");

            using spare_bits_view = bit_view<unsigned char, 1u, CHAR_BIT>;

            static bool verify() noexcept
            {
                static const Boolean true_value(true);
                static const Boolean false_value(false);
                return *reinterpret_cast<const unsigned char*>(&true_value) == 1u
                       && *reinterpret_cast<const unsigned char*>(&false_value) == 0u;
            }

        public:
            static constexpr auto spare_bits = spare_bits_view::size();

            static void clear(bool& obj) noexcept
            {
                put(obj, 0);
            }

            static std::uintmax_t extract(bool obj) noexcept
            {
                DEBUG_ASSERT(verify(), detail::precondition_handler{},
                             "memory layout of boolean incorrect");
                return spare_bits_view(reinterpret_cast<unsigned char&>(obj)).extract();
            }

            static void put(bool& obj, std::uintmax_t spare_bits) noexcept
            {
                DEBUG_ASSERT(verify(), detail::precondition_handler{},
                             "memory layout of boolean incorrect");
                spare_bits_view(reinterpret_cast<unsigned char&>(obj)).put(spare_bits);
            }
        };

        /// The spare bits of a (non-void) pointer.
        ///
        /// It will use the alignment bits at the end (which are always zero) as spare bits.
        template <typename Pointer>
        class spare_bits_traits_pointer
        {
            static_assert(std::is_pointer<Pointer>::value, "not a pointer type");

            using element_type = typename std::remove_pointer<Pointer>::type;
            static_assert(!std::is_void<element_type>::value, "void pointer not allowed");

            using spare_bits_view =
                bit_view<std::uintptr_t, 0, detail::ilog2_ceil(alignof(element_type))>;

        public:
            static constexpr auto spare_bits = spare_bits_view::size();

            static void clear(Pointer& object) noexcept
            {
                put(object, 0u);
            }

            static std::uintmax_t extract(Pointer object) noexcept
            {
                auto int_value = reinterpret_cast<std::uintptr_t>(object);
                return spare_bits_view(int_value).extract();
            }

            static void put(Pointer& object, std::uintmax_t bits) noexcept
            {
                auto int_value = reinterpret_cast<std::uintptr_t>(object);
                spare_bits_view(int_value).put(bits);
                object = reinterpret_cast<Pointer>(int_value);
            }
        };

        //=== spare bits traits ===//
        /// The spare bits traits of a given type.
        ///
        /// It will not have any spare bits by default.
        template <typename T>
        struct spare_bits_traits : spare_bits_traits_default<T>
        {
        };

        /// Enable the boolean tombstone traits for `bool`.
        template <>
        struct spare_bits_traits<bool> : spare_bits_traits_boolean<bool>
        {
        };

        /// Enable the pointer spare_bits traits for pointer types.
        template <typename T>
        struct spare_bits_traits<T*> : spare_bits_traits_pointer<T*>
        {
        };

        /// Disable the pointer spare bits traits for `void*`.
        /// \group spare_bits_void
        template <>
        struct spare_bits_traits<void*> : spare_bits_traits_default<void*>
        {
        };
        /// \group spare_bits_void
        template <>
        struct spare_bits_traits<const void*> : spare_bits_traits_default<const void*>
        {
        };
        /// \group spare_bits_void
        template <>
        struct spare_bits_traits<volatile void*> : spare_bits_traits_default<volatile void*>
        {
        };
        /// \group spare_bits_void
        template <>
        struct spare_bits_traits<const volatile void*>
        : spare_bits_traits_default<const volatile void*>
        {
        };

        //=== spare bits traits algorithm ===//
        /// \returns The number of spare bits in the object.
        template <typename T>
        constexpr std::size_t spare_bits() noexcept
        {
            return spare_bits_traits<T>::spare_bits;
        }

        /// \returns A copy of the object where the spare bits are all zero.
        template <typename T>
        T extract_object(const T& obj)
        {
            T result(obj);
            spare_bits_traits<T>::clear(result);
            return result;
        }

        namespace detail
        {
            template <typename T>
            class modifier
            {
            public:
                explicit modifier(T& obj) noexcept
                : ptr_(&obj), spare_(spare_bits_traits<T>::extract(obj))
                {
                    spare_bits_traits<T>::clear(obj);
                }

                modifier(const modifier&) = delete;
                modifier& operator=(const modifier&) = delete;

                ~modifier() noexcept
                {
                    spare_bits_traits<T>::put(*ptr_, spare_);
                }

                T& operator*() const noexcept
                {
                    return *ptr_;
                }

                T* operator->() const noexcept
                {
                    return ptr_;
                }

            private:
                T*             ptr_;
                std::uintmax_t spare_;
            };
        } // namespace detail

        /// \returns A pointer-like type to the given object where the spare bits are zero.
        /// The object can be savely accessed through that pointer and as long as the pointer lives.
        template <typename T>
        detail::modifier<T> modify_object(T& obj) noexcept
        {
            return detail::modifier<T>(obj);
        }

        /// \returns The spare bits of the object.
        template <typename T>
        std::uintmax_t extract_spare_bits(const T& obj) noexcept
        {
            return spare_bits_traits<T>::extract(obj);
        }

        /// \effects Sets the spare bits of the object.
        template <typename T>
        void put_spare_bits(T& obj, std::uintmax_t bits) noexcept
        {
            DEBUG_ASSERT((bit_view<std::uintmax_t, spare_bits<T>(),
                                   sizeof(std::uintmax_t) * CHAR_BIT>(bits)
                              .extract())
                             == 0u,
                         detail::precondition_handler{}, "attempt to set more bits than can fit");
            spare_bits_traits<T>::put(obj, bits);
        }

        /// \effects Clears the spare bits.
        template <typename T>
        void clear_spare_bits(T& obj) noexcept
        {
            spare_bits_traits<T>::clear(obj);
        }
    } // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_SPARE_BITS_HPP_INCLUDED
