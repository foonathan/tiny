// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_TINY_PAIR_HPP_INCLUDED
#define FOONATHAN_TINY_TINY_PAIR_HPP_INCLUDED

#include <utility>

#include <foonathan/tiny/detail/select_integer.hpp>
#include <foonathan/tiny/spare_bits.hpp>

namespace foonathan
{
    namespace tiny
    {
        namespace detail
        {
            template <bool UseSpareBits, typename Big, typename Integer>
            class tiny_pair_impl_storage;

            template <typename Big, typename Integer>
            class tiny_pair_impl_storage<true, Big, Integer>
            {
            public:
                using integer_type  = Integer;
                using is_compressed = std::true_type;

                explicit tiny_pair_impl_storage(Big big) : storage_(big) {}

                tiny_pair_impl_storage(const tiny_pair_impl_storage& other) : storage_(other.big())
                {
                    set_integer(other.integer());
                }

                tiny_pair_impl_storage(tiny_pair_impl_storage&& other) noexcept(
                    std::is_nothrow_move_constructible<Big>::value)
                : storage_(std::move(other))
                {
                    set_integer(other.integer());
                }

                ~tiny_pair_impl_storage() noexcept
                {
                    clear_spare_bits(storage_);
                }

                tiny_pair_impl_storage& operator=(const tiny_pair_impl_storage& other)
                {
                    *modify_big() = other.big();
                    set_integer(other.integer());
                    return *this;
                }

                tiny_pair_impl_storage& operator=(tiny_pair_impl_storage&& other) noexcept(
                    std::is_nothrow_move_assignable<Big>::value)
                {
                    *modify_big() = std::move(*other.modify_big());
                    set_integer(other.integer());
                    return *this;
                }

                Big big() const
                {
                    return extract_object(storage_);
                }

                detail::modifier<Big> modify_big() noexcept
                {
                    return modify_object(storage_);
                }

                Integer integer() const
                {
                    return static_cast<Integer>(extract_spare_bits(storage_));
                }

                void set_integer(Integer i) noexcept
                {
                    put_spare_bits(storage_, i);
                }

            private:
                Big storage_;
            };

            template <typename Big, typename Integer>
            class tiny_pair_impl_storage<false, Big, Integer>
            {
                static_assert(!std::is_same<Integer, void>::value,
                              "should have used the other specialization below");

            public:
                using integer_type  = Integer;
                using is_compressed = std::false_type;

                explicit tiny_pair_impl_storage(Big big) : big_(std::move(big)) {}

                const Big& big() const noexcept
                {
                    return big_;
                }

                Big* modify_big() noexcept
                {
                    return &big_;
                }

                Integer integer() const noexcept
                {
                    return int_;
                }

                void set_integer(Integer i) noexcept
                {
                    int_ = i;
                }

            private:
                Big     big_;
                Integer int_;
            };

            template <typename Big>
            class tiny_pair_impl_storage<true, Big, void>
            {
            public:
                using integer_type  = unsigned char;
                using is_compressed = std::true_type;

                explicit tiny_pair_impl_storage(Big big) : big_(std::move(big)) {}

                const Big& big() const noexcept
                {
                    return big_;
                }

                Big* modify_big() noexcept
                {
                    return &big_;
                }

                integer_type integer() const noexcept
                {
                    return 0u;
                }

                void set_integer(integer_type) noexcept {}

            private:
                Big big_;
            };

            template <typename Big, std::size_t TinyBits>
            using tiny_pair_impl_storage_t =
                tiny_pair_impl_storage<TinyBits <= spare_bits<Big>(), Big,
                                       detail::uint_least_n_t<TinyBits>>;
        } // namespace detail

        /// Implementation helper for tiny pairs.
        ///
        /// It contains an object of type `Big` and an `TinyBits` bits.
        /// Whenever possible, the tiny bits will use the spare bits of `Big`.
        /// \notes This is just meant as an implementation helper for higher-level types,
        /// it is not intended to be used in interfaces.
        template <typename Big, std::size_t TinyBits>
        class tiny_pair_impl
        {
            using storage = detail::tiny_pair_impl_storage_t<Big, TinyBits>;

        public:
            using big_type      = Big;
            using integer_type  = typename storage::integer_type;
            using is_compressed = typename storage::is_compressed;

            /// \effects Creates a pair of the two values.
            explicit tiny_pair_impl(Big big, integer_type i) : storage_(std::move(big))
            {
                storage_.set_integer(i);
            }

            /// \returns A copy or const reference to the big object.
            auto big() const -> decltype(std::declval<storage>().big())
            {
                return storage_.big();
            }

            /// \returns A pointer-like type that can be used to modify the big object.
            auto modify_big() noexcept -> decltype(std::declval<storage>().modify_big())
            {
                return storage_.modify_big();
            }

            /// \returns The integer value that is being stored.
            integer_type integer() const noexcept
            {
                return storage_.integer();
            }

            /// \effects Sets the integer value.
            void set_integer(integer_type i) noexcept
            {
                storage_.set_integer(i);
            }

        private:
            storage storage_;
        };
    } // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_TINY_PAIR_HPP_INCLUDED
