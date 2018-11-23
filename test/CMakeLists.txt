# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# get Catch
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
    file(DOWNLOAD
         https://raw.githubusercontent.com/catchorg/Catch2/master/single_include/catch2/catch.hpp
         ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp
         STATUS status
         LOG log)

    list(GET status 0 status_code)
    list(GET status 1 status_string)

    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "error downloading catch: ${status_string}"
                "${log}")
    endif()
endif()

# define dummy target with all the options
# can't link to foonathan_tiny, as it adds a SYSTEM interface
add_library(foonathan_tiny_test_base INTERFACE)
target_sources(foonathan_tiny_test_base INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp)
target_include_directories(foonathan_tiny_test_base INTERFACE
                               ${CMAKE_CURRENT_SOURCE_DIR}
                               ${CMAKE_CURRENT_SOURCE_DIR}/../include
                           SYSTEM INTERFACE
                               ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(foonathan_tiny_test_base INTERFACE cxx_std_11)
target_link_libraries(foonathan_tiny_test_base INTERFACE debug_assert)
target_compile_definitions(foonathan_tiny_test_base INTERFACE
                           FOONATHAN_TINY_ENABLE_ASSERTIONS=1
                           FOONATHAN_TINY_ENABLE_PRECONDITIONS=1)
target_compile_options(foonathan_tiny_test_base INTERFACE
                        # clang/GCC warnings
                        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
                           -pedantic-errors -Werror -Wall -Wextra -Wconversion -Wsign-conversion>
                        # disable noexcept type warning on GCC
                        $<$<CXX_COMPILER_ID:GNU>: -Wno-noexcept-type>
                        # MSVC warnings
                        $<$<CXX_COMPILER_ID:MSVC>:
                           /WX /W4>)
# enable sanitizer
target_compile_options(foonathan_tiny_test_base INTERFACE
                        $<$<CXX_COMPILER_ID:Clang>:
                            -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer>)
target_link_libraries(foonathan_tiny_test_base INTERFACE
                        $<$<CXX_COMPILER_ID:Clang>:
                            -fsanitize=address -fsanitize=undefined>)

# unit tests
set(tests
    detail/ilog2.cpp
    bit_view.cpp
    check_size.cpp
    optional_impl.cpp
    pointer_tiny_storage.cpp
    padding_tiny_storage.cpp
    padding_traits.cpp
    poiner_variant_impl.cpp
    tombstone_traits.cpp
    tagged_union_impl.cpp
    tiny_types.cpp
    tiny_storage.cpp)

add_executable(foonathan_tiny_test ${tests})
target_link_libraries(foonathan_tiny_test PUBLIC foonathan_tiny_test_base)
add_test(NAME test COMMAND foonathan_tiny_test)

