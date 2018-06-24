option(grammar_build_tests "Build all of grammar's tests." ON)


cmake_minimum_required(VERSION 2.8)

##Project Information
project(grammar CXX C)


# Location of additional cmake modules
set(CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        ${PROJECT_SOURCE_DIR}/cmake)

# Guard against in-source builds and bad build-type strings
include(ConfigSafeGuards)


# Global Setup
set(CMAKE_CXX_STANDARD 14)


# Set common include folder for module
set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)

include_directories(${COMMON_INCLUDES})


# Library Setup
set(SOURCE_FILES include/grammar/re_pair.h
        src/re_pair.cpp
        src/repair/basics.h
        src/repair/basics.cpp
        src/repair/array.h
        src/repair/array.cpp
        src/repair/records.h
        src/repair/records.cpp
        src/repair/heap.h
        src/repair/heap.cpp
        src/repair/hash.h
        src/repair/hash.cpp)

add_library(grammar ${SOURCE_FILES})
target_link_libraries(grammar)


# Define helper functions and macros used.
include(cmake/internal_utils.cmake)


if (grammar_build_tests)
    enable_testing()

    include(ConfigGTest)

    cxx_test_with_flags_and_args(re_pair_test "" "gtest;gtest_main;grammar" "" test/re_pair_test.cpp)
endif ()