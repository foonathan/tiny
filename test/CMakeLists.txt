# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

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

set(tests
        bit_view.cpp
        optional_impl.cpp
        padding_traits.cpp
        poiner_variant_impl.cpp
        spare_bits_traits.cpp
        tombstone_traits.cpp
        tiny_pair.cpp
        tiny_types.cpp
        tiny_pointer_storage.cpp
        tiny_type_storage.cpp)

add_executable(foonathan_tiny_test
               test.cpp
               ${tests})
target_include_directories(foonathan_tiny_test SYSTEM PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(foonathan_tiny_test PUBLIC foonathan_tiny)
set_target_properties(foonathan_tiny_test PROPERTIES CXX_STANDARD 11)

add_test(NAME test COMMAND foonathan_tiny_test)
