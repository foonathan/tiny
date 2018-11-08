# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

add_executable(foonathan_tiny_storage tiny_storage.cpp)
target_link_libraries(foonathan_tiny_storage PUBLIC foonathan_tiny)

add_executable(foonathan_tiny_custom_tiny_type custom_tiny_type.cpp)
target_link_libraries(foonathan_tiny_custom_tiny_type PUBLIC foonathan_tiny)

add_executable(foonathan_tiny_tombstone tombstone.cpp)
target_link_libraries(foonathan_tiny_tombstone PUBLIC foonathan_tiny)

add_executable(foonathan_tiny_tombstone_manual tombstone.cpp)
target_link_libraries(foonathan_tiny_tombstone_manual PUBLIC foonathan_tiny)
target_compile_definitions(foonathan_tiny_tombstone_manual PUBLIC TOMBSTONE_MANUAL)
