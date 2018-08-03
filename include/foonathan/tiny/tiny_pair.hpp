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

            // TODO: make this one triviallly Xable if Big is trivially Xable etc.
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
                : storage_(std::move(other.storage_))
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

                void set_integer(integer_type i) noexcept
                {
                    DEBUG_ASSERT(i == 0u, precondition_handler{},
                                 "integer uses more bits than available");
                }

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

        namespace detail
        {
            namespace adl_get_detail
            {
                using std::get;

                template <std::size_t I, class Tuple>
                auto adl_get(Tuple&& tuple) -> decltype(get<I>(std::forward<Tuple>(tuple)))
                {
                    return get<I>(std::forward<Tuple>(tuple));
                }
            } // namespace adl_get_detail

            using adl_get_detail::adl_get;

            template <typename PairLike, typename First, typename Second>
            using is_matching_pair = std::integral_constant<
                bool, std::tuple_size<PairLike>::value == 2u
                          && std::is_convertible<typename std::tuple_element<0u, PairLike>::type,
                                                 First>::value
                          && std::is_convertible<typename std::tuple_element<1u, PairLike>::type,
                                                 Second>::value>;

            template <typename PairLike, typename First, typename Second>
            using enable_matching_pair = typename std::enable_if<
                is_matching_pair<typename std::decay<PairLike>::type, First, Second>::value>::type;

        } // namespace detail

        //=== tiny bool pair ===//
        /// A pair of a `T` and a `bool`, compressed whenever possible.
        template <typename T>
        class tiny_bool_pair
        {
            using impl = tiny_pair_impl<T, 1u>;

        public:
            using first_type  = T;
            using second_type = bool;

            using is_compressed = typename impl::is_compressed;

            //=== constructors ===//
            /// \effects Creates a pair of a default constructed `T` and `false`.
            tiny_bool_pair() : tiny_bool_pair(T(), 0u) {}

            /// \effects Creates a pair of the given value and boolean.
            /// \group two_args
            tiny_bool_pair(const T& first, bool second)
            : impl_(first, static_cast<typename impl::integer_type>(second))
            {
            }
            /// \group two_args
            tiny_bool_pair(T&& first, bool second)
            : impl_(std::move(first), static_cast<typename impl::integer_type>(second))
            {
            }

            /// \effects Creates it from a type with tuple interface.
            /// \requires It must be a tuple of two types where the first one is convertible to `T` and the second to `bool`.
            template <class PairLike, typename = detail::enable_matching_pair<PairLike, T, bool>>
            tiny_bool_pair(PairLike&& pair)
            : tiny_bool_pair(detail::adl_get<0>(std::forward<PairLike>(pair)),
                             detail::adl_get<1>(std::forward<PairLike>(pair)))
            {
            }

            /// \effects Assigns it from a type with tuple interface.
            /// \requires It must be a tuple of two types where the first one is convertible to `T` and the second to `bool`.
            template <class PairLike, typename = detail::enable_matching_pair<PairLike, T, bool>>
            tiny_bool_pair& operator=(PairLike&& pair)
            {
                *modify_first() = detail::adl_get<0u>(std::forward<PairLike>(pair));
                set_second(detail::adl_get<1u>(std::forward<PairLike>(pair)));
                return *this;
            }

            //=== access ===//
            /// \returns The non-boolean value.
            auto first() const noexcept -> decltype(std::declval<impl>().big())
            {
                return impl_.big();
            }

            /// \returns A pointer-like type that can be used to modify the non-boolean value.
            auto modify_first() noexcept -> decltype(std::declval<impl>().modify_big())
            {
                return impl_.modify_big();
            }

            /// \effects Sets the non-boolean value.
            void set_first(T value)
            {
                *impl_.modify_big() = std::move(value);
            }

            /// \returns The boolean value.
            bool second() const noexcept
            {
                // need to mask other bits away, they could have been used to store other things
                return (impl_.integer() & 0x1) == 1u;
            }

            /// \effects Sets the boolean value.
            void set_second(bool b) noexcept
            {
                // get everything but the boolean bit
                auto spare_bits = impl_.integer() & static_cast<typename impl::integer_type>(~0x1);
                // combine it with the boolean bit
                impl_.set_integer(static_cast<typename impl::integer_type>(spare_bits | b));
            }

        private:
            tiny_pair_impl<T, 1u> impl_;

            friend spare_bits_traits<tiny_bool_pair<T>>;
        };

        /// \returns A tiny bool pair created from the two value.
        template <typename T>
        tiny_bool_pair<typename std::decay<T>::type> make_tiny_pair(T&& first, bool second)
        {
            return {std::forward<T>(first), second};
        }

        /// \effects Compares a tiny bool pair by comparing both values.
        /// \group tiny_bool_pair_comp
        template <typename T>
        bool operator==(const tiny_bool_pair<T>& lhs, const tiny_bool_pair<T>& rhs)
        {
            return lhs.second() == rhs.second() && lhs.first() == rhs.first();
        }
        /// \group tiny_bool_pair_comp
        template <typename T>
        bool operator!=(const tiny_bool_pair<T>& lhs, const tiny_bool_pair<T>& rhs)
        {
            return !(lhs == rhs);
        }
        /// \group tiny_bool_pair_comp
        template <typename T>
        bool operator<(const tiny_bool_pair<T>& lhs, const tiny_bool_pair<T>& rhs)
        {
            return lhs.first() < rhs.first()
                   || (!(rhs.first() < lhs.first()) && lhs.second() < rhs.second());
        }
        /// \group tiny_bool_pair_comp
        template <typename T>
        bool operator<=(const tiny_bool_pair<T>& lhs, const tiny_bool_pair<T>& rhs)
        {
            return !(rhs < lhs);
        }
        /// \group tiny_bool_pair_comp
        template <typename T>
        bool operator>(const tiny_bool_pair<T>& lhs, const tiny_bool_pair<T>& rhs)
        {
            return rhs < lhs;
        }
        /// \group tiny_bool_pair_comp
        template <typename T>
        bool operator>=(const tiny_bool_pair<T>& lhs, const tiny_bool_pair<T>& rhs)
        {
            return !(lhs < rhs);
        }

        namespace detail
        {
            template <std::size_t I, typename TinyPair>
            struct tiny_bool_pair_get;

            template <typename T>
            struct tiny_bool_pair_get<0u, tiny_bool_pair<T>>
            {
                using type = T;

                static T get(const tiny_bool_pair<T>& pair)
                {
                    return pair.first();
                }
            };

            template <typename T>
            struct tiny_bool_pair_get<1u, tiny_bool_pair<T>>
            {
                using type = bool;

                static bool get(const tiny_bool_pair<T>& pair)
                {
                    return pair.second();
                }
            };
        } // namespace detail

        /// \returns The `I`th type of the pair.
        template <std::size_t I, typename T>
        auto get(const tiny_bool_pair<T>& pair) ->
            typename detail::tiny_bool_pair_get<I, tiny_bool_pair<T>>::type
        {
            return detail::tiny_bool_pair_get<I, tiny_bool_pair<T>>::get(pair);
        }

        /// The spare bits traits of the tiny bool pair.
        ///
        /// It exposes the other spare bits of `T` or of the integer used to store the boolean.
        template <typename T>
        struct spare_bits_traits<tiny_bool_pair<T>>
        {
            static constexpr std::size_t spare_bits =
                tiny::spare_bits<T>() >= 1u ?
                    // use remaining spare bits of T
                    tiny::spare_bits<T>() - 1u :
                    // use remaining bits of integer type
                    sizeof(typename tiny_bool_pair<T>::impl::integer_type) * CHAR_BIT - 1u;

            static void clear(tiny_bool_pair<T>&) noexcept
            {
                // no need to do anything, other integer bits don't matter
            }

            static std::uintmax_t extract(const tiny_bool_pair<T>& pair) noexcept
            {
                return static_cast<std::uintmax_t>(pair.impl_.integer() >> 1);
            }

            static void put(tiny_bool_pair<T>& pair, std::uintmax_t bits) noexcept
            {
                // the new spare integer value doesn't use the lowermost bit
                auto spare_integer =
                    static_cast<typename tiny_bool_pair<T>::impl::integer_type>(bits << 1u);
                // the boolean bit
                auto bool_bit =
                    static_cast<typename tiny_bool_pair<T>::impl::integer_type>(pair.second());
                // combine the two
                pair.impl_.set_integer(spare_integer | bool_bit);
            }
        };
    } // namespace tiny
} // namespace foonathan

namespace std
{
    template <typename T>
    struct tuple_size<foonathan::tiny::tiny_bool_pair<T>> : std::integral_constant<std::size_t, 2u>
    {
    };

    template <std::size_t I, typename T>
    struct tuple_element<I, foonathan::tiny::tiny_bool_pair<T>>
    {
        using type = typename foonathan::tiny::detail::tiny_bool_pair_get<
            I, foonathan::tiny::tiny_bool_pair<T>>::type;
    };
} // namespace std

#endif // FOONATHAN_TINY_TINY_PAIR_HPP_INCLUDED
