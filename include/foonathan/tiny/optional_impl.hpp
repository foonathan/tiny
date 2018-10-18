// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_OPTIONAL_IMPL_HPP_INCLUDED
#define FOONATHAN_TINY_OPTIONAL_IMPL_HPP_INCLUDED

#include <type_traits>

#include <foonathan/tiny/tombstone.hpp>

namespace foonathan
{
namespace tiny
{
    template <typename T>
    class optional_impl;

    namespace detail
    {
        template <bool HasTombstone, typename T>
        class optional_has_value_impl;

        template <typename T>
        class optional_has_value_impl<true, T>
        {
        protected:
            void store_value() noexcept {}

            void store_none(void* storage) noexcept
            {
                create_tombstone<T>(storage, empty_tombstone());
            }

            bool has_value_impl(const void* storage) const noexcept
            {
                return tombstone_index<T>(storage) != empty_tombstone();
            }

        private:
            static constexpr std::size_t empty_tombstone() noexcept
            {
                return tombstone_count<T>() - 1;
            }
        };

        template <typename T>
        class optional_has_value_impl<false, T>
        {
        protected:
            void store_value() noexcept
            {
                has_value_ = true;
            }

            void store_none(void*) noexcept
            {
                has_value_ = false;
            }

            bool has_value_impl(const void*) const noexcept
            {
                return has_value_;
            }

        private:
            bool has_value_ = false;

            friend struct tiny::tombstone_traits<optional_impl<T>>;
            friend struct tiny::spare_bits_traits<optional_impl<T>>;
        };
    } // namespace detail

    /// The storage implementation of an optional type that uses tombstones whenever possible.
    ///
    /// It is just a low-level implementation helper for an actual optional.
    /// A proper optional type should be built on top of it.
    template <typename T>
    class optional_impl : detail::optional_has_value_impl<(tombstone_count<T>() > 0u), T>
    {
    public:
        using value_type    = T;
        using is_compressed = std::integral_constant<bool, (tombstone_count<T>() > 0u)>;

        //=== constructors ===//
        /// \effects Creates an empty optional.
        optional_impl() noexcept
        {
            this->store_none(storage_ptr());
        }

        /// \effects Does nothing.
        /// \notes This will leak the value if there is one stored currently.
        ~optional_impl() noexcept = default;

        optional_impl(const optional_impl&) = delete;
        optional_impl& operator=(const optional_impl&) = delete;

        //=== mutators ===//
        /// \effects Creates a value by forwarding the arguments.
        /// \requires `has_value() == false`.
        template <typename... Args>
        void create_value(Args&&... args)
        {
            DEBUG_ASSERT(!has_value(), detail::precondition_handler{});
            ::new (storage_ptr()) T(std::forward<Args>(args)...);
            this->store_value();
        }

        /// \effects Destoys the currently stored value.
        /// \requires `has_value() == true`.
        void destroy_value() noexcept
        {
            value().~T();
            this->store_none(storage_ptr());
        }

        //=== accessors ===//
        /// \returns Whether or not the optional currently stores a value.
        bool has_value() const noexcept
        {
            return this->has_value_impl(storage_ptr());
        }

        /// \returns A reference to the value currently stored inside the optional.
        /// \requires `has_value() == true`.
        /// \group value
        T& value() noexcept
        {
            DEBUG_ASSERT(has_value(), detail::precondition_handler{});
            return *static_cast<T*>(storage_ptr());
        }
        /// \group value
        const T& value() const noexcept
        {
            DEBUG_ASSERT(has_value(), detail::precondition_handler{});
            return *static_cast<T*>(storage_ptr());
        }

    private:
        void* storage_ptr() noexcept
        {
            return &storage_;
        }
        const void* storage_ptr() const noexcept
        {
            return &storage_;
        }

        using storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
        storage storage_;

        friend struct tombstone_traits<optional_impl<T>>;
    };

    /// The tombstone traits for [tiny::optional_impl]().
    ///
    /// It will expose additional tombstones of the type.
    template <typename T>
    struct tombstone_traits<optional_impl<T>>
    {
        using overlap_spare_bits =
            typename std::conditional<optional_impl<T>::is_compressed::value,
                                      // no spare bits if it is compressed
                                      std::false_type, tombstone_overlaps_spare_bits<bool>>::type;

        static constexpr std::size_t tombstone_count
            = optional_impl<T>::is_compressed::value
                  ?
                  // compressed, we have one tombstone fewer
                  tiny::tombstone_count<T>() - 1u
                  :
                  // not compressed, we can use the bool for the tombstones
                  tiny::tombstone_count<bool>();

        static void create_tombstone(void* memory, std::size_t tombstone_index) noexcept
        {
            create_tombstone(typename optional_impl<T>::is_compressed{}, memory, tombstone_index);
        }

        static std::size_t tombstone_index(const void* memory) noexcept
        {
            return tombstone_index(typename optional_impl<T>::is_compressed{}, memory);
        }

    private:
        static void create_tombstone(std::true_type, void* memory, std::size_t tombstone_index)
        {
            // create a new empty optional
            auto opt = ::new (memory) optional_impl<T>();
            // create a tombstone of the given index inside the storage of that optional
            // as we are in the compressed case, we don't need to set any boolean
            // and as we've used the highest tombstone index as empty optional,
            // there is no need to adjust the index
            tiny::create_tombstone<T>(opt->storage_ptr(), tombstone_index);
        }
        static std::size_t tombstone_index(std::true_type, const void* memory)
        {
            // can always cast to an optional
            auto opt = static_cast<const optional_impl<T>*>(memory);
            // again we can just return the tombstone index of T
            return tiny::tombstone_index<T>(opt->storage_ptr());
        }

        static void create_tombstone(std::false_type, void* memory, std::size_t tombstone_index)
        {
            // create a new empty optional
            auto opt = ::new (memory) optional_impl<T>();
            // create the boolean tombstone in the has value field
            tiny::create_tombstone<bool>(&opt->has_value_, tombstone_index);
        }
        static std::size_t tombstone_index(std::false_type, const void* memory)
        {
            // can always cast to  an optional
            auto opt = static_cast<const optional_impl<T>*>(memory);
            // forward to the boolean field again
            return tiny::tombstone_index<bool>(&opt->has_value_);
        }
    };

    /// The spare bits implementation of [tiny::optional_impl]().
    /// \notes As `optional_impl` is not copyable, it cannot actually be used directly as a type
    /// with spare bits. But the proper optional type can delegate to these traits.
    template <typename T>
    struct spare_bits_traits<optional_impl<T>>
    {
        static constexpr std::size_t spare_bits
            = optional_impl<T>::is_compressed::value ? 0u : tiny::spare_bits<bool>();

        static void clear(optional_impl<T>& opt)
        {
            clear(typename optional_impl<T>::is_compressed{}, opt);
        }

        static std::uintmax_t extract(const optional_impl<T>& opt)
        {
            return extract(typename optional_impl<T>::is_compressed{}, opt);
        }

        static void put(optional_impl<T>& opt, std::uintmax_t bits)
        {
            put(typename optional_impl<T>::is_compressed{}, opt, bits);
        }

    private:
        static void           clear(std::true_type, optional_impl<T>&) {}
        static std::uintmax_t extract(std::true_type, const optional_impl<T>&)
        {
            return 0u;
        }
        static void put(std::true_type, optional_impl<T>&, std::uintmax_t) {}

        static void clear(std::false_type, optional_impl<T>& opt)
        {
            clear_spare_bits(opt.has_value_);
        }
        static std::uintmax_t extract(std::false_type, const optional_impl<T>& opt)
        {
            return extract_spare_bits(opt.has_value_);
        }
        static void put(std::false_type, optional_impl<T>& opt, std::uintmax_t bits)
        {
            put_spare_bits(opt.has_value_, bits);
        }
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_OPTIONAL_IMPL_HPP_INCLUDED
