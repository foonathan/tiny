// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_PADDING_TINY_STORAGE_HPP_INCLUDED
#define FOONATHAN_TINY_PADDING_TINY_STORAGE_HPP_INCLUDED

#include <cstring>

#include <foonathan/tiny/padding_traits.hpp>
#include <foonathan/tiny/tiny_storage.hpp>

namespace foonathan
{
namespace tiny
{
    namespace detail
    {
        //=== padded_holder ===//
        // the compiler may not copy the padding bytes when copying the type,
        // so we need to do that ourselves
        template <bool IsTrivial, class Padded>
        class padded_holder;

        template <class Padded>
        class padded_holder<true, Padded>
        {
        public:
            padded_holder() = default;

            template <typename... Args>
            void construct(Args&&... args)
            {
                padded_ = Padded(static_cast<Args&&>(args)...);
                clear_bits(padding_of(padded_));
            }

            padded_holder(const padded_holder& other) noexcept
            {
                std::memcpy(&padded_, &other.padded_, sizeof(Padded));
            }

            ~padded_holder() = default;

            padded_holder& operator=(const padded_holder& other) noexcept
            {
                std::memcpy(&padded_, &other.padded_, sizeof(Padded));
                return *this;
            }

            Padded& get() noexcept
            {
                return padded_;
            }
            const Padded& get() const noexcept
            {
                return padded_;
            }

        private:
            Padded padded_;
        };

        template <class Padded>
        class padded_holder<false, Padded>
        {
        public:
            padded_holder() = default;

            template <typename... Args>
            void construct(Args&&... args)
            {
                ::new (get_pointer()) Padded(static_cast<Args&&>(args)...);
                clear_bits(padding_of(get()));
            }

            padded_holder(const padded_holder& other)
            {
                ::new (get_pointer()) Padded(other.get());
                assign_padding(other.get());
            }

            padded_holder(padded_holder&& other) noexcept(
                std::is_nothrow_move_constructible<Padded>::value)
            {
                ::new (get_pointer()) Padded(static_cast<Padded&&>(other.get()));
                assign_padding(other.get());
            }

            ~padded_holder() noexcept
            {
                get().~Padded();
            }

            padded_holder& operator=(const padded_holder& other)
            {
                get() = other.get();
                assign_padding(other.get());
                return *this;
            }

            padded_holder& operator=(padded_holder&& other) noexcept(
                std::is_nothrow_move_assignable<Padded>::value)
            {
                get() = static_cast<Padded&&>(other.get());
                assign_padding(other.get());
                return *this;
            }

            Padded& get() noexcept
            {
                return *static_cast<Padded*>(get_pointer());
            }
            const Padded& get() const noexcept
            {
                return *static_cast<const Padded*>(get_pointer());
            }

        private:
            void assign_padding(const Padded& other) noexcept
            {
                auto dest = padding_of(get());
                auto src  = padding_of(other);
                copy_bits(dest, src);
            }

            void* get_pointer() noexcept
            {
                return &storage_;
            }
            const void* get_pointer() const noexcept
            {
                return &storage_;
            }

            typename std::aligned_storage<sizeof(Padded), alignof(Padded)>::type storage_;
        };

        template <class Padded>
        using padded_holder_for = padded_holder<std::is_trivial<Padded>::value, Padded>;

        //=== padded_storage_policy ===//
        template <class Padded, class... TinyTypes>
        class padded_storage_policy
        {
            static constexpr auto compressed_size = padding_bit_size<Padded>();
            static constexpr auto total_size      = total_bit_size<TinyTypes...>();
            static constexpr auto remaining_size
                = total_size > compressed_size ? total_size - compressed_size : 0;

            struct compressed_storage
            {
                padded_holder_for<Padded> obj;

                template <typename... Args>
                compressed_storage(Args&&... args) : obj(static_cast<Args&&>(args)...)
                {}

                Padded& object() noexcept
                {
                    return obj.get();
                }
                const Padded& object() const noexcept
                {
                    return obj.get();
                }

                padding_view_t<Padded> view() noexcept
                {
                    return padding_of(obj.get());
                }
                padding_view_t<const Padded> view() const noexcept
                {
                    return padding_of(obj.get());
                }
            };

            struct uncompressed_storage
            {
                padded_holder_for<Padded>         obj;
                tiny_storage_type<remaining_size> storage;

                template <typename... Args>
                uncompressed_storage(Args&&... args) : obj(static_cast<Args&&>(args)...)
                {}

                using storage_view = bit_view<tiny_storage_type<remaining_size>, 0, last_bit>;
                using storage_cview
                    = bit_view<const tiny_storage_type<remaining_size>, 0, last_bit>;

                Padded& object() noexcept
                {
                    return obj.get();
                }
                const Padded& object() const noexcept
                {
                    return obj.get();
                }

                joined_bit_view<padding_view_t<Padded>, storage_view> view() noexcept
                {
                    return join_bit_views(padding_of(obj.get()), storage_view(storage));
                }
                joined_bit_view<padding_view_t<const Padded>, storage_cview> view() const noexcept
                {
                    return join_bit_views(padding_of(obj.get()), storage_cview(storage));
                }
            };

            using storage = typename std::conditional<remaining_size == 0, compressed_storage,
                                                      uncompressed_storage>::type;

            storage storage_;

        private:
            using is_compressed = std::integral_constant<bool, remaining_size == 0>;

            padded_storage_policy() = default;

            auto storage_view() noexcept -> decltype(std::declval<storage>().view())
            {
                return storage_.view();
            }
            auto storage_view() const noexcept -> decltype(std::declval<const storage>().view())
            {
                return storage_.view();
            }

            friend basic_tiny_storage<padded_storage_policy<Padded, TinyTypes...>, TinyTypes...>;

        public:
            template <typename... Args>
            void construct(Args&&... args)
            {
                storage_.obj.construct(static_cast<Args&&>(args)...);
            }

            Padded& object() noexcept
            {
                return storage_.obj.get();
            }
            const Padded& object() const noexcept
            {
                return storage_.obj.get();
            }
        };
    } // namespace detail

    /// Stores an object of the specified type and the tiny types.
    ///
    /// The tiny types are stored in padding bits of the object whenever possible.
    template <class Padded, class... TinyTypes>
    class padding_tiny_storage
    : public basic_tiny_storage<detail::padded_storage_policy<Padded, TinyTypes...>, TinyTypes...>
    {
    public:
        using padded_type = Padded;

        /// Default constructor.
        /// \effects Creates a storage where the object is default constructed and the tiny types
        /// have all bits set to zero.
        padding_tiny_storage()
        {
            this->storage_policy().construct();
        }

        /// \effects Creates a storage where the object is `obj` and the tiny types have all bits
        /// set to zero.
        explicit padding_tiny_storage(padded_type obj)
        {
            this->storage_policy().construct(static_cast<padded_type&&>(obj));
        }

        /// \effects Creates a storage where the object `obj` and the tiny types are created from
        /// their object types.
        padding_tiny_storage(padded_type obj, typename TinyTypes::object_type... tiny)
        : padding_tiny_storage(detail::make_index_sequence<sizeof...(TinyTypes)>{},
                               static_cast<padded_type&&>(obj), tiny...)
        {}

        /// \returns A reference to the object.
        /// \group object
        padded_type& object() noexcept
        {
            return this->storage_policy().object();
        }
        /// \group object
        const padded_type& object() const noexcept
        {
            return this->storage_policy().object();
        }

    private:
        template <std::size_t... Indices>
        padding_tiny_storage(detail::index_sequence<Indices...>, padded_type obj,
                             typename TinyTypes::object_type... tiny)
        {
            this->storage_policy().construct(static_cast<padded_type&&>(obj));

            bool for_each[] = {(this->template at<Indices>() = tiny, true)..., true};
            (void)for_each;
        }
    };
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_PADDING_TINY_STORAGE_HPP_INCLUDED
