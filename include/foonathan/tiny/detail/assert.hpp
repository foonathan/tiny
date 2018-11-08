// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_DETAIL_ASSERT_HPP_INCLUDED
#define FOONATHAN_TINY_DETAIL_ASSERT_HPP_INCLUDED

#include <debug_assert.hpp>

#ifndef FOONATHAN_TINY_ENABLE_ASSERTIONS
#    define FOONATHAN_TINY_ENABLE_ASSERTIONS 0
#endif

#ifndef FOONATHAN_TINY_ENABLE_PRECONDITIONS
#    ifdef NDEBUG
#        define FOONATHAN_TINY_ENABLE_PRECONDITIONS 0
#    else
#        define FOONATHAN_TINY_ENABLE_PRECONDITIONS 1
#    endif
#endif

namespace foonathan
{
namespace tiny
{
    namespace detail
    {
        struct assert_handler
        : debug_assert::default_handler,
          debug_assert::set_level<static_cast<unsigned>(FOONATHAN_TINY_ENABLE_ASSERTIONS)>
        {};

        struct precondition_handler
        : debug_assert::default_handler,
          debug_assert::set_level<static_cast<unsigned>(FOONATHAN_TINY_ENABLE_PRECONDITIONS)>
        {};
    } // namespace detail
} // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_DETAIL_ASSERT_HPP_INCLUDED
