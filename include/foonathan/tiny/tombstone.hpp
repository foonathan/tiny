// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TOMBSTONE_HPP_INCLUDED
#define FOONATHAN_TINY_TOMBSTONE_HPP_INCLUDED

#include <cstddef>
#include <new>

#include <foonathan/tiny/padding_traits.hpp>
#include <foonathan/tiny/tiny_int.hpp>
#include <foonathan/tiny/tiny_storage.hpp>

namespace foonathan
{
namespace tiny
{
    /// \exclude
    namespace tombstone_detail
    {
        template <typename T>
        union storage_type_trivial
        {
            char uninitialized;
            T    object;

            storage_type_trivial() noexcept {}
            storage_type_trivial(const storage_type_trivial&) = delete;
            storage_type_trivial& operator=(const storage_type_trivial&) = delete;
            ~storage_type_trivial() noexcept                             = default;
        };

        template <typename T>
        union storage_type_non_trivial
        {
            char uninitialized;
            T    object;

            storage_type_non_trivial() noexcept {}
            storage_type_non_trivial(const storage_type_non_trivial&) = delete;
            storage_type_non_trivial& operator=(const storage_type_non_trivial&) = delete;
            ~storage_type_non_trivial() noexcept {}
        };

        template <typename T>
        using storage_type_for =
            typename std::conditional<std::is_trivially_destructible<T>::value,
                                      storage_type_trivial<T>, storage_type_non_trivial<T>>::type;
    } // namespace tombstone_detail

    /// The tombstone traits of a given type.
    ///
    /// The default implementation provides no tombstones.
    template <typename T, typename = void>
    struct tombstone_traits
    {
        /// The object type, i.e. `T`.
        ///
        /// This is the type that is conceptually stored.
        using object_type = T;

        /// The type that is the storage for both tombstone and object type.
        ///
        /// It must be a type that is default constructible,
        /// "trivially" destructible and doesn't need to be copy constructible or assignable.
        ///
        /// \notes The destructor of this type shouldn't actually destroy something.
        /// But it would be reasonable to use a `union` of `T` and some dummy member.
        /// However, the destructor then has to be defined as a no-op, so isn't actually trivial
        /// anymore. But it still does nothing.
        using storage_type = tombstone_detail::storage_type_for<T>;

        /// A reference to the object type.
        ///
        /// This is either a plain old reference or a proxy.
        using reference       = T&;
        using const_reference = const T&;

        /// The number of tombstones that are available.
        static constexpr std::size_t tombstone_count = 0u;

        /// \effects Creates the tombstone with the specified index in the storage.
        /// \requires The storage must currently contain nothing and `tombstone_index <
        /// tombstone_count`.
        static void create_tombstone(storage_type& storage, std::size_t tombstone_index) noexcept
        {
            (void)storage;
            (void)tombstone_index;
        }

        /// \effects Creates an object in the storage.
        /// \requires The storage must currently contain nothing or a tombstone.
        template <typename... Args>
        static void create_object(storage_type& storage, Args&&... args)
        {
            ::new (static_cast<void*>(&storage.object)) T(static_cast<Args>(args)...);
        }

        /// \effects Destroys the object currently stored in the storage.
        /// \requires An object must currently be stored.
        static void destroy_object(storage_type& storage) noexcept
        {
            storage.object.~T();
        }

        /// \returns The index of the currently stored tombstone, or an invalid index if an object
        /// is stored. \requires The storage must either contain an object or a tombstone and not be
        /// in the uninitialized state.
        static std::size_t get_tombstone(const storage_type& storage) noexcept
        {
            (void)storage;
            return 0u;
        }

        /// \returns A reference to the object currently stored in the storage.
        /// \requires The storage must store an object.
        /// \group get_object
        static reference get_object(storage_type& storage) noexcept
        {
            return storage.object;
        }
        /// \group get_object
        static const_reference get_object(const storage_type& storage) noexcept
        {
            return storage.object;
        }
    };

    //=== tombstone_traits_simple ===//
    /// \exclude
    namespace tombstone_detail
    {
        template <typename T, class Tombstone>
        union dual_storage_type_trivial
        {
            Tombstone tombstone;
            T         object;

            dual_storage_type_trivial() noexcept {}
            dual_storage_type_trivial(const dual_storage_type_trivial&) = delete;
            dual_storage_type_trivial& operator=(const dual_storage_type_trivial&) = delete;
            ~dual_storage_type_trivial() noexcept                                  = default;
        };

        template <typename T, class Tombstone>
        union dual_storage_type_non_trivial
        {
            Tombstone tombstone;
            T         object;

            dual_storage_type_non_trivial() noexcept {}
            dual_storage_type_non_trivial(const dual_storage_type_non_trivial&) = delete;
            dual_storage_type_non_trivial& operator=(const dual_storage_type_non_trivial&) = delete;
            ~dual_storage_type_non_trivial() noexcept {}
        };

        template <typename T, class Tombstone>
        using dual_storage_type_for =
            typename std::conditional<std::is_trivially_destructible<T>::value,
                                      dual_storage_type_trivial<T, Tombstone>,
                                      dual_storage_type_non_trivial<T, Tombstone>>::type;
    } // namespace tombstone_detail

    /// A tombstone traits implementation of common boilerplate.
    ///
    /// The `TombstoneType` is a trivially copyable, nothrow default constructible type that is
    /// layout compatible with `T`. It can store all the tombstones and valid objects.
    ///
    /// When specializing the traits, inherit from it and provide the following members:
    /// * `static constexpr std::size_t tombstone_count`: the number of tombstones
    /// * `static void create_tombstone_impl(void* memory, std::size_t index)`:
    ///   creates an object of `TombstoneType` in `memory` representing the specified tombstone
    /// * `static std::size_t get_tombstone_impl(const TombstoneType& tombstone_or_object)`:
    ///   returns the index of the tombstone or an invalid index if it is no tombstone
    template <typename T, class TombstoneType = T>
    class tombstone_traits_simple
    {
        static_assert(is_layout_compatible<T, TombstoneType>::value,
                      "TombstoneType must be layout compatible");
        static_assert(std::is_default_constructible<TombstoneType>::value,
                      "TombstoneType must be nothrow default constructible");
        static_assert(std::is_trivially_copyable<TombstoneType>::value,
                      "TombstoneType must be trivially copyable");

    public:
        using object_type     = T;
        using storage_type    = tombstone_detail::dual_storage_type_for<T, TombstoneType>;
        using reference       = T&;
        using const_reference = const T&;

        static void create_tombstone(storage_type& storage, std::size_t index) noexcept
        {
            tombstone_traits<T>::create_tombstone_impl(&storage.tombstone, index);
        }

        template <typename... Args>
        static void create_object(storage_type& storage, Args&&... args)
        {
            ::new (static_cast<void*>(&storage.object)) T(static_cast<Args>(args)...);
        }

        static void destroy_object(storage_type& storage) noexcept
        {
            storage.object.~T();
        }

        static std::size_t get_tombstone(const storage_type& storage) noexcept
        {
            return tombstone_traits<T>::get_tombstone_impl(storage.tombstone);
        }

        static reference get_object(storage_type& storage) noexcept
        {
            return storage.object;
        }
        static const_reference get_object(const storage_type& storage) noexcept
        {
            return storage.object;
        }
    };

    //=== tombstone_traits using padding ===//
    /// A tombstone traits implementation that uses the padding bits to mark the tombstones.
    ///
    /// `Padded` is the type that will get a tombstone,
    /// `TombstoneType` is a layout compatible, nothrow default-constructible and trivially copyable
    /// type that has a specialization of the padding traits.
    template <typename T, class TombstoneType = T>
    class tombstone_traits_padded
    {
        static_assert(is_layout_compatible<T, TombstoneType>::value,
                      "TombstoneType must be layout compatible");
        static_assert(std::is_default_constructible<TombstoneType>::value,
                      "TombstoneType must be nothrow default constructible");
        static_assert(std::is_trivially_copyable<TombstoneType>::value,
                      "TombstoneType must be trivially copyable");
        static_assert(padding_bit_size<TombstoneType>() > 0, "doesn't actually have padding");

        static constexpr std::size_t tombstone_bits
            = padding_bit_size<TombstoneType>() > sizeof(std::size_t) * CHAR_BIT - 1
                  ? sizeof(std::size_t) * CHAR_BIT - 1
                  : padding_bit_size<TombstoneType>();

    public:
        using object_type = T;

        using storage_type = tombstone_detail::dual_storage_type_for<T, TombstoneType>;

        using reference       = T&;
        using const_reference = const T&;

        // - 1 because all zero is the actual object
        static constexpr std::size_t tombstone_count = (1ull << tombstone_bits) - 1;

        static void create_tombstone(storage_type& storage, std::size_t tombstone_index) noexcept
        {
            storage.tombstone = {};
            padding_of(storage.tombstone)
                .template subview<0, tombstone_bits>()
                .put(tombstone_index + 1);
        }

        template <typename... Args>
        static void create_object(storage_type& storage, Args&&... args)
        {
            ::new (static_cast<void*>(&storage.object)) T(static_cast<Args>(args)...);
            padding_of(storage.tombstone).template subview<0, tombstone_bits>().put(0);
        }

        static void destroy_object(storage_type& storage) noexcept
        {
            storage.object.~T();
        }

        static std::size_t get_tombstone(const storage_type& storage) noexcept
        {
            auto data
                = padding_of(storage.tombstone).template subview<0, tombstone_bits>().extract();
            // if data == 0: no tombstone
            // else: data - 1 is index
            // we can unconditionally subtract one, if zero, it overflows and we have an invalid
            // index
            return data - 1;
        }

        static reference get_object(storage_type& storage) noexcept
        {
            return storage.object;
        }
        static const_reference get_object(const storage_type& storage) noexcept
        {
            return storage.object;
        }
    };

    /// Specialization of the tombstone traits for types with padding bits and that are valid
    /// tombstone types.
    template <typename T>
    struct tombstone_traits<T,
                            typename std::enable_if<(padding_bit_size<T>() > 0)
                                                    && std::is_default_constructible<T>::value
                                                    && std::is_trivially_copyable<T>::value>::type>
    : tombstone_traits_padded<T>
    {};

    //=== tombstone traits for tiny types ===//
    /// Specialization of the tombstone traits for tiny types.
    template <class TinyType>
    struct tombstone_traits<TinyType, typename std::enable_if<is_tiny_type<TinyType>::value>::type>
    {
    private:
        static_assert(TinyType::bit_size() < CHAR_BIT, "TinyType is not actually tiny");

        using tombstone_type = tiny_unsigned<CHAR_BIT - TinyType::bit_size(), std::size_t>;

    public:
        using object_type = typename TinyType::object_type;

        using storage_type = tiny_storage<TinyType, tombstone_type>;

        using reference       = decltype(std::declval<storage_type&>().template at<0>());
        using const_reference = decltype(std::declval<const storage_type&>().template at<0>());

        static constexpr std::size_t tombstone_count = (1ull << tombstone_type::bit_size()) - 1u;

        static void create_tombstone(storage_type& storage, std::size_t tombstone_index) noexcept
        {
            storage.template at<1>() = tombstone_index + 1;
        }

        template <typename... Args>
        static void create_object(storage_type& storage, Args&&... args)
        {
            storage.template at<0>() = object_type(static_cast<Args>(args)...);
            storage.template at<1>() = 0;
        }

        static void destroy_object(storage_type&) noexcept {}

        static std::size_t get_tombstone(const storage_type& storage) noexcept
        {
            // if data == 0: no tombstone
            // else: data - 1 is index
            // we can unconditionally subtract one, if zero, it overflows and we have an invalid
            // index
            return storage.template at<1>() - 1;
        }

        static reference get_object(storage_type& storage) noexcept
        {
            return storage.template at<0>();
        }
        static const_reference get_object(const storage_type& storage) noexcept
        {
            return storage.template at<0>();
        }
    };

    /// Specialization of the tombstone traits for `bool`.
    ///
    /// This does not behave like `tiny_bool` would, but instead works without proxy types.
    /// \notes It assumes that `true` has bit pattern `00...01` and `false` pattern `00...00`.
    template <>
    struct tombstone_traits<bool>
    {
        using object_type     = bool;
        using storage_type    = bool;
        using reference       = bool&;
        using const_reference = const bool&;

        static constexpr std::size_t tombstone_count = (1u << (CHAR_BIT - 1)) - 1;

        static void create_tombstone(storage_type& storage, std::size_t index) noexcept
        {
            // add one so that the higher bits are never zero
            reinterpret_cast<unsigned char&>(storage)
                = static_cast<unsigned char>((index + 1) << 1);
        }

        static void create_object(storage_type& storage, bool obj) noexcept
        {
            storage = obj;
        }

        static void destroy_object(storage_type&) noexcept {}

        static std::size_t get_tombstone(const storage_type& storage) noexcept
        {
            auto bits = std::size_t(reinterpret_cast<const unsigned char&>(storage) >> 1);
            // unconditionally subtract one, will overflow correctly
            return bits - 1;
        }

        static reference get_object(storage_type& storage) noexcept
        {
            return storage;
        }
        static const_reference get_object(const storage_type& storage) noexcept
        {
            return storage;
        }
    };

    //=== tombstone_traits for pointers ===//
    /// \exclude
    template <typename T, std::size_t Alignment>
    struct aligned_obj;

    namespace tombstone_detail
    {
        template <typename Pointer, std::size_t Alignment>
        struct tombstone_traits_ptr
        {
            using object_type  = Pointer;
            using storage_type = std::uintptr_t;

            using reference       = object_type&;
            using const_reference = const object_type&;

            // alignment zero is valid
            static constexpr std::size_t tombstone_count = Alignment - 1;

            static void create_tombstone(storage_type& storage,
                                         std::size_t   tombstone_index) noexcept
            {
                storage = static_cast<std::uintptr_t>(tombstone_index + 1);
            }

            static void create_object(storage_type& storage, object_type ptr) noexcept
            {
                storage = reinterpret_cast<std::uintptr_t>(ptr);
            }
            static void destroy_object(storage_type&) noexcept {}

            static std::size_t get_tombstone(storage_type storage) noexcept
            {
                // storage % Alignment: just the lower order bits
                // valid pointer: 0, subtract one overflows and we have an invalid index
                // otherwise: tombstone index + 1
                return (storage % Alignment) - 1;
            }

            static reference get_object(storage_type& storage) noexcept
            {
                return reinterpret_cast<reference>(storage);
            }
            static const_reference get_object(const storage_type& storage) noexcept
            {
                return reinterpret_cast<reference>(storage);
            }
        };

        template <typename T, std::size_t Alignment, std::size_t Dummy>
        struct tombstone_traits_ptr<aligned_obj<T, Alignment>*, Dummy>
        : tombstone_traits_ptr<T*, Alignment>
        {};

        template <typename T>
        struct is_cv_void : std::is_same<typename std::remove_cv<T>::type, void>
        {};

        template <typename T>
        struct is_void_ptr : is_cv_void<typename std::remove_pointer<T>::type>
        {};
    } // namespace tombstone_detail

    /// Specialization of the tombstone traits for pointers.
    /// It will use the invalid alignments.
    template <typename Pointer>
    struct tombstone_traits<
        Pointer, typename std::enable_if<std::is_pointer<Pointer>::value
                                         && !tombstone_detail::is_void_ptr<Pointer>::value>::type>
    : tombstone_detail::tombstone_traits_ptr<Pointer,
                                             alignof(typename std::remove_pointer<Pointer>::type)>
    {};
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TOMBSTONE_HPP_INCLUDED
