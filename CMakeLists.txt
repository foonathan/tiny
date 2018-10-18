# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.1)
project(foonathan_tiny)

set(detail_header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/detail/assert.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/detail/ilog2.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/detail/select_integer.hpp
    )
set(header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/aligned_ptr.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/bit_field.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/bit_view.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/optional_impl.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/pointer_variant_impl.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/spare_bits.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/tombstone.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/tiny_pair.hpp
    )
add_library(foonathan_tiny INTERFACE)
target_sources(foonathan_tiny INTERFACE ${detail_header_files} ${header_files})
target_include_directories(foonathan_tiny INTERFACE include ../debug_assert/)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    enable_testing()
    add_subdirectory(test)
endif()

# temporary, for testing
add_executable(foonathan_tiny_main main.cpp)
target_link_libraries(foonathan_tiny_main PUBLIC foonathan_tiny)
set_target_properties(foonathan_tiny_main PROPERTIES CXX_STANDARD 11)
