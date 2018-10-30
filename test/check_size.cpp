// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/tiny/check_size.hpp>

using namespace foonathan::tiny;

namespace
{
struct foo
{};
static_assert(check_size<foo, 1>(), "");
FOONATHAN_TINY_CHECK_SIZE(foo, 1);
static_assert(check_alignment<foo, 1>(), "");
FOONATHAN_TINY_CHECK_ALIGNMENT(foo, 1);
} // namespace
