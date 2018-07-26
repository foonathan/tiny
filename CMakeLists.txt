# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.1)
project(foonathan_tiny)

add_executable(foonathan_tiny_main main.cpp)
set_target_properties(foonathan_tiny_main PROPERTIES CXX_STANDARD 11)
