########################################################################
# CMake build script for Grammar Library.

option(grammar_build_tests "Build all of grammar's tests." ON)

option(grammar_build_tools "Build all of grammar's tools." ON)

option(grammar_build_benchmarks "Build all of grammar's benchmarks." ON)

option(grammar_install "Enable installation of grammar. (Projects embedding grammar may want to turn this OFF.)" ON)


cmake_minimum_required(VERSION 2.8)


########################################################################
#
# Project-wide settings
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


########################################################################
#
# Grammar library
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
        src/repair/hash.cpp
        include/grammar/algorithm.h
        include/grammar/slp.h
        include/grammar/slp_metadata.h
        include/grammar/slp_interface.h
        include/grammar/slp_helper.h
        include/grammar/sampled_slp.h
        include/grammar/io.h
        include/grammar/differential_slp.h)

add_library(grammar ${SOURCE_FILES})

set(LIBS sdsl)
target_link_libraries(grammar ${LIBS})


########################################################################
#
# Install rules
if (grammar_install)
    if (CMAKE_VERSION VERSION_LESS 2.8.5)
        set(CMAKE_INSTALL_BINDIR "bin" CACHE STRING "User executables (bin)")
        set(CMAKE_INSTALL_LIBDIR "lib${LIB_SUFFIX}" CACHE STRING "Object code libraries (lib)")
        set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE STRING "C header files (include)")
        mark_as_advanced(CMAKE_INSTALL_BINDIR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR)
    else()
        include(GNUInstallDirs)
    endif()

    install(TARGETS grammar
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    install(DIRECTORY "${grammar_SOURCE_DIR}/include/grammar"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif ()


########################################################################
#
# Grammar's own tests.

# Define helper functions and macros used.
include(cmake/internal_utils.cmake)


if (grammar_build_tests)
    enable_testing()

    include(ConfigGTest)

    cxx_test_with_flags_and_args(complete_tree_test "" "gtest;gtest_main;grammar" "" test/complete_tree_test.cpp)

    cxx_test_with_flags_and_args(re_pair_test "" "gtest;gtest_main;gflags;grammar" "--datapath=${CMAKE_SOURCE_DIR}/test/data" test/re_pair_test.cpp)

    cxx_test_with_flags_and_args(slp_test "" "gtest;gtest_main;grammar" "" test/slp_test.cpp)

    cxx_test_with_flags_and_args(merge_sets_test "" "gtest;gtest_main;grammar" "" test/merge_sets_test.cpp)

    cxx_test_with_flags_and_args(slp_metadata_test "" "gtest;gtest_main;grammar;sdsl" "" test/slp_metadata_test.cpp)

    cxx_test_with_flags_and_args(sampled_slp_test "" "gtest;gtest_main;grammar" "" test/sampled_slp_test.cpp)
endif ()


find_package(Boost COMPONENTS filesystem system REQUIRED)

if (grammar_build_tools)
    cxx_executable_with_flags(build_dslp_span_sums "" "gflags;grammar;${Boost_LIBRARIES}" tool/build_dslp_span_sums.cpp)

    cxx_executable_with_flags(build_dslp_samples "" "gflags;grammar;${Boost_LIBRARIES}" tool/build_dslp_samples.cpp)
endif ()


if (grammar_build_benchmarks)
    include(ConfigGBenchmark)

    find_package(Boost COMPONENTS filesystem system REQUIRED)

    cxx_executable_with_flags(chunks_bm "" "gflags;benchmark;grammar;${Boost_LIBRARIES}" benchmark/chunks_bm.cpp)
    cxx_executable_with_flags(differential_slp_bm "" "gflags;benchmark;grammar;${Boost_LIBRARIES}" benchmark/differential_slp_bm.cpp)
endif ()
