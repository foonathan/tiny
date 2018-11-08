// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_OPTIONAL_IMPL_HPP_INCLUDED
#define FOONATHAN_TINY_OPTIONAL_IMPL_HPP_INCLUDED

#include <type_traits>

#include <foonathan/tiny/tiny_bool.hpp>
#include <foonathan/tiny/tombstone.hpp>

namespace foonathan
{
namespace tiny
{
    /// \exclude
    namespace opt_detail
    {
        template <typename T>
        struct compressed_optional
        {
            using traits = tombstone_traits<T>;
            typename traits::storage_type storage;

            static constexpr std::size_t empty_tombstone() noexcept
            {
                return traits::tombstone_count - 1u;
            }

            void store_value_flag() noexcept {}

            void store_none_flag() noexcept
            {
                traits::create_tombstone(storage, empty_tombstone());
            }

            bool has_value() const noexcept
            {
                return traits::get_tombstone(storage) != empty_tombstone();
            }
        };

        template <typename T>
        struct uncompressed_optional
        {
            using traits = tombstone_traits<T>;
            typename traits::storage_type                                     storage;
            tiny_storage<tiny_bool, tiny_unsigned<CHAR_BIT - 1, std::size_t>> flag;

            void store_value_flag() noexcept
            {
                flag.at<0>() = true;
            }

            void store_none_flag() noexcept
            {
                flag.at<0>() = false;
            }

            bool has_value() const noexcept
            {
                return flag.at<0>();
            }
        };

        template <typename T>
        struct compressed_traits;
        template <typename T>
        struct uncompressed_traits;
    } // namespace opt_detail

    /// The storage implementation of an optional type that uses tombstones whenever possible.
    ///
    /// It is just a low-level implementation helper for an actual optional.
    /// A proper optional type should be built on top of it.
    template <typename T>
    class optional_impl
    {
        using traits = tombstone_traits<T>;

    public:
        using value_type    = typename traits::object_type;
        using is_compressed = std::integral_constant<bool, (traits::tombstone_count > 0u)>;

        //=== constructors ===//
        /// \effects Creates an empty optional.
        optional_impl() noexcept
        {
            impl_.store_none_flag();
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
            traits::create_object(impl_.storage, static_cast<Args>(args)...);
            impl_.store_value_flag();
        }

        /// \effects Destoys the currently stored value.
        /// \requires `has_value() == true`.
        void destroy_value() noexcept
        {
            traits::destroy_object(impl_.storage);
            impl_.store_none_flag();
        }

        //=== accessors ===//
        /// \returns Whether or not the optional currently stores a value.
        bool has_value() const noexcept
        {
            return impl_.has_value();
        }

        /// \returns A reference to the value currently stored inside the optional.
        /// \requires `has_value() == true`.
        /// \group value
        auto value() noexcept -> typename traits::reference
        {
            DEBUG_ASSERT(has_value(), detail::precondition_handler{});
            return traits::get_object(impl_.storage);
        }
        /// \group value
        auto value() const noexcept -> typename traits::const_reference
        {
            DEBUG_ASSERT(has_value(), detail::precondition_handler{});
            return traits::get_object(impl_.storage);
        }

    private:
        typename std::conditional<is_compressed::value, opt_detail::compressed_optional<T>,
                                  opt_detail::uncompressed_optional<T>>::type impl_;

        friend opt_detail::compressed_traits<T>;
        friend opt_detail::uncompressed_traits<T>;
    };

    namespace opt_detail
    {
        template <typename T>
        struct compressed_traits
        {
            using object_type     = optional_impl<T>;
            using storage_type    = optional_impl<T>;
            using reference       = object_type&;
            using const_reference = const object_type&;

            static constexpr std::size_t tombstone_count = tombstone_traits<T>::tombstone_count - 1;

            static void create_tombstone(storage_type& storage,
                                         std::size_t   tombstone_index) noexcept
            {
                tombstone_traits<T>::create_tombstone(storage.impl_.storage, tombstone_index);
            }

            // optional_impl only has a default constructor
            static void create_object(storage_type& storage)
            {
                // this overrides the tombstone so it means no tombstone at this level
                storage.impl_.store_none_flag();
            }

            static void destroy_object(storage_type&) noexcept {}

            static std::size_t get_tombstone(const storage_type& storage) noexcept
            {
                // if non-empty optional: doesn't store a tombstone, so returns invalid index
                // if empty optional: stores tombstone with index tombstone_count, so invalid index
                // otherwise: another tombstone index
                return tombstone_traits<T>::get_tombstone(storage.impl_.storage);
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

        template <typename T>
        struct uncompressed_traits
        {
            using object_type     = optional_impl<T>;
            using storage_type    = optional_impl<T>;
            using reference       = object_type&;
            using const_reference = const object_type&;

            static constexpr std::size_t tombstone_count = (1u << (CHAR_BIT - 1)) - 1;

            static void create_tombstone(storage_type& storage,
                                         std::size_t   tombstone_index) noexcept
            {
                storage.impl_.flag.template at<1>() = tombstone_index + 1;
            }

            // optional_impl only has a default constructor
            static void create_object(storage_type& storage)
            {
                // this overrides the tombstone so it means no tombstone at this level
                storage.impl_.store_none_flag();
                storage.impl_.flag.template at<1>() = 0;
            }

            static void destroy_object(storage_type&) noexcept {}

            static std::size_t get_tombstone(const storage_type& storage) noexcept
            {
                return storage.impl_.flag.template at<1>() - 1;
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
    } // namespace opt_detail

    /// Specialization of the tombstone traits for [tiny::optional_impl]().
    template <typename T>
    struct tombstone_traits<optional_impl<T>>
    : std::conditional<(tombstone_traits<T>::tombstone_count > 0), opt_detail::compressed_traits<T>,
                       opt_detail::uncompressed_traits<T>>::type
    {};
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_OPTIONAL_IMPL_HPP_INCLUDED
