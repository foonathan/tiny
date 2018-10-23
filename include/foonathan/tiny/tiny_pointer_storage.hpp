// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TINY_POINTER_STORAGE_HPP_INCLUDED
#define FOONATHAN_TINY_TINY_POINTER_STORAGE_HPP_INCLUDED

#include <foonathan/tiny/detail/ilog2.hpp>
#include <foonathan/tiny/tiny_type_storage.hpp>

namespace foonathan
{
namespace tiny
{
    namespace detail
    {
        template <std::size_t Alignment, class... TinyTypes>
        class pointer_storage_policy
        {
            static constexpr auto compressed_size = detail::ilog2_ceil(Alignment);

            using ptr_view  = bit_view<std::uintptr_t, 0, compressed_size>;
            using ptr_cview = bit_view<const std::uintptr_t, 0, compressed_size>;

            static constexpr auto total_size     = total_bit_size<TinyTypes...>();
            static constexpr auto remaining_size = total_size - compressed_size;

            struct compressed_storage
            {
                std::uintptr_t ptr;

                ptr_view view() noexcept
                {
                    return ptr_view(ptr);
                }
                ptr_cview view() const noexcept
                {
                    return ptr_cview(ptr);
                }
            };

            struct uncompressed_storage
            {
                std::uintptr_t                    ptr;
                tiny_storage_type<remaining_size> storage;

                using storage_view = bit_view<tiny_storage_type<remaining_size>, 0, last_bit>;
                using storage_cview
                    = bit_view<const tiny_storage_type<remaining_size>, 0, last_bit>;

                joined_bit_view<ptr_view, storage_view> view() noexcept
                {
                    return join_bit_views(ptr_view(ptr), storage_view(storage));
                }
                joined_bit_view<ptr_cview, storage_cview> view() const noexcept
                {
                    return join_bit_views(ptr_cview(ptr), storage_cview(storage));
                }
            };

            using storage = typename std::conditional<remaining_size == 0, compressed_storage,
                                                      uncompressed_storage>::type;

            storage storage_;

        private:
            using is_compressed = std::integral_constant<bool, remaining_size == 0>;

            pointer_storage_policy() noexcept = default;

            auto storage_view() noexcept -> decltype(std::declval<storage>().view())
            {
                return storage_.view();
            }
            auto storage_view() const noexcept -> decltype(std::declval<const storage>().view())
            {
                return storage_.view();
            }

            friend basic_tiny_type_storage<pointer_storage_policy<Alignment, TinyTypes...>,
                                           TinyTypes...>;

        public:
            template <typename T>
            void set_pointer(T* ptr) noexcept
            {
                auto as_int = reinterpret_cast<std::uintptr_t>(ptr);
                DEBUG_ASSERT((are_cleared_bits<0, compressed_size>(as_int)),
                             detail::precondition_handler{}, "invalid alignment of pointer");
                auto tiny_bits = extract_bits<0, compressed_size>(storage_.ptr);
                storage_.ptr   = as_int | tiny_bits;
            }

            template <typename T>
            T* get_pointer() const noexcept
            {
                auto int_value = storage_.ptr;
                clear_bits<0, compressed_size>(int_value);
                return reinterpret_cast<T*>(int_value);
            }
        };

        template <typename T, std::size_t Alignment, class... TinyTypes>
        class pointer_proxy
        {
        public:
            pointer_proxy(int, pointer_storage_policy<Alignment, TinyTypes...>* storage) noexcept
            : storage_(storage)
            {}

            operator T*() const noexcept
            {
                return storage_->template get_pointer<T>();
            }

            const pointer_proxy& operator=(T* pointer) const noexcept
            {
                storage_->set_pointer(pointer);
                return *this;
            }
            const pointer_proxy& operator+=(std::ptrdiff_t diff) const noexcept
            {
                storage_->set_pointer(storage_->template get_pointer<T>() + diff);
                return *this;
            }
            const pointer_proxy& operator-=(std::ptrdiff_t diff) const noexcept
            {
                storage_->set_pointer(storage_->template get_pointer<T>() - diff);
                return *this;
            }

            const pointer_proxy& operator++() const noexcept
            {
                return *this += 1;
            }
            T* operator++(int) const noexcept
            {
                auto ptr = storage_->template get_pointer<T>();
                *this += 1;
                return ptr;
            }

            const pointer_proxy& operator--() const noexcept
            {
                return *this -= 1;
            }
            T* operator--(int) const noexcept
            {
                auto ptr = storage_->template get_pointer<T>();
                *this -= 1;
                return ptr;
            }

        private:
            pointer_storage_policy<Alignment, TinyTypes...>* storage_;
        };
    } // namespace detail

    /// Tag type that specifies that an object of type `T` will be aligned for `Alignment`.
    ///
    /// It is used for [tiny::tiny_pointer_storage]() and [tiny::pointer_variant_impl]().
    template <typename T, std::size_t Alignment>
    struct aligned_obj
    {
        using type                             = T;
        static constexpr std::size_t alignment = Alignment;
    };

    namespace detail
    {
        template <typename T>
        struct alignment_traits
        {
            using type                             = T;
            static constexpr std::size_t alignment = alignof(T);
        };

        template <typename T, std::size_t Alignment>
        struct alignment_traits<aligned_obj<T, Alignment>>
        {
            using type                             = T;
            static constexpr std::size_t alignment = Alignment;
        };
    } // namespace detail

    /// The alignment of an object of the given type.
    ///
    /// If the type is [tiny::aligned_obj](), the alignment is the alignment specified there.
    /// Otherwise it is `alignof(T)`.
    template <typename T>
    constexpr std::size_t alignment_of()
    {
        return detail::alignment_traits<T>::alignment;
    }

    /// Stores a pointer to `T` and the specified tiny types.
    ///
    /// It will use the bits from the pointer that are always zero due to alignment to store the
    /// tiny bits whenever possible, using additional storage only if necessary.
    ///
    /// Pass [tiny::aligned_obj]() instead of `T` if you know that the object you need to point to
    /// has a given over-alignment.
    template <typename T, typename... TinyTypes>
    class tiny_pointer_storage
    : public basic_tiny_type_storage<
          detail::pointer_storage_policy<alignment_of<T>(), TinyTypes...>, TinyTypes...>
    {
        static_assert(!std::is_same<typename std::remove_cv<T>::type, void>::value,
                      "void pointers have no alignment, wrap them in aligned_obj instead");

    public:
        using value_type   = typename detail::alignment_traits<T>::type;
        using pointer_type = value_type*;

        /// Default constructor.
        /// \effects Creates a storage where the pointer is `nullptr` and the tiny types have all
        /// bits set to zero.
        tiny_pointer_storage() noexcept
        {
            pointer() = nullptr;
        }

        /// \effects Creates a storage where the pointer is `ptr` and the tiny types have all bits
        /// set to zero.
        explicit tiny_pointer_storage(pointer_type ptr) noexcept
        {
            pointer() = ptr;
        }

        /// \effects Creates a storage where the pointer is `ptr` and the tiny types are created
        /// from their object types.
        tiny_pointer_storage(pointer_type ptr, typename TinyTypes::object_type... tiny) noexcept
        : basic_tiny_type_storage<detail::pointer_storage_policy<alignment_of<T>(), TinyTypes...>,
                                  TinyTypes...>(tiny...)
        {
            pointer() = ptr;
        }

        /// \returns A proxy that behaves like a mutable reference to the stored pointer.
        detail::pointer_proxy<value_type, alignment_of<T>(), TinyTypes...> pointer() noexcept
        {
            return {0, &this->storage_policy()};
        }
        /// \returns The stored pointer.
        T* pointer() const noexcept
        {
            return this->storage_policy().template get_pointer<T>();
        }
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TINY_POINTER_STORAGE_HPP_INCLUDED
