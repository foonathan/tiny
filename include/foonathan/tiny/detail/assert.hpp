// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_TINY_DETAIL_ASSERT_HPP_INCLUDED
#define FOONATHAN_TINY_DETAIL_ASSERT_HPP_INCLUDED

#include <debug_assert.hpp>

namespace foonathan
{
    namespace tiny
    {
        namespace detail
        {
            // TODO: customizable level
            struct precondition_handler : debug_assert::default_handler,
                                          debug_assert::set_level<unsigned(-1)>
            {
            };

            struct assert_handler : debug_assert::default_handler,
                                    debug_assert::set_level<unsigned(-1)>
            {
            };
        } // namespace detail
    }     // namespace tiny
} // namespace foonathan

#endif // FOONATHAN_TINY_DETAIL_ASSERT_HPP_INCLUDED
