# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.8)
project(foonathan_tiny VERSION 0.0.0)

include(external/external.cmake)

# source files
set(detail_header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/detail/assert.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/detail/ilog2.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/detail/index_sequence.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/detail/select_integer.hpp
    )
set(header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/bit_view.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/check_size.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/enum_traits.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/optional_impl.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/padding_tiny_storage.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/padding_traits.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/pointer_tiny_storage.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/pointer_variant_impl.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/tagged_union_impl.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/tombstone.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/tiny_bool.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/tiny_enum.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/tiny_flag_set.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/tiny_int.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/tiny_storage.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/tiny/tiny_type.hpp
    )

# main target
add_library(foonathan_tiny INTERFACE)
add_library(foonathan::foonathan_tiny ALIAS foonathan_tiny)
target_sources(foonathan_tiny INTERFACE $<BUILD_INTERFACE:${detail_header_files} ${header_files}>)
target_compile_features(foonathan_tiny INTERFACE cxx_std_11)
target_include_directories(foonathan_tiny SYSTEM INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
                           $<INSTALL_INTERFACE:include/>)
target_link_libraries(foonathan_tiny INTERFACE debug_assert)

# installation
if(NOT dependency_via_submodule)
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(foonathan_tiny-config-version.cmake COMPATIBILITY ExactVersion)
    install(TARGETS foonathan_tiny EXPORT foonathan_tiny_targets
            INCLUDES DESTINATION include)
    install(EXPORT foonathan_tiny_targets
            DESTINATION lib/cmake/foonathan_tiny
            FILE foonathan_tiny-targets.cmake
            NAMESPACE foonathan::)
    install(DIRECTORY include/
            DESTINATION include)
    install(FILES foonathan_tiny-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/foonathan_tiny-config-version.cmake
            DESTINATION lib/cmake/foonathan_tiny)
else()
    message(STATUS "Dependency installed via submodule, installation unavailable")
endif()

# subdirectories
option(FOONATHAN_TINY_BUILD_TEST "build tests of foonathan/tiny" OFF)
if(${FOONATHAN_TINY_BUILD_TEST} OR (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    enable_testing()
    add_subdirectory(test)
endif()

option(FOONATHAN_TINY_BUILD_EXAMPLE "build examples of foonathan/tiny" OFF)
if(${FOONATHAN_TINY_BUILD_EXAMPLE} OR (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    add_subdirectory(example)
endif()
