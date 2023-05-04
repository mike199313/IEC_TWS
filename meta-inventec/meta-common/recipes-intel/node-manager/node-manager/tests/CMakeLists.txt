#  INTEL CONFIDENTIAL
#
#  Copyright 2022 Intel Corporation.
#
#  This software and the related documents are Intel copyrighted materials,
#  and your use of them is governed by the express license under which they
#  were provided to you ("License"). Unless the License provides otherwise,
#  you may not use, modify, copy, publish, distribute, disclose or transmit
#  this software or the related documents without Intel's prior written
#  permission.
#
#  This software and the related documents are provided as is, with
#  no express or implied warranties, other than those that are expressly
#  stated in the License.

cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

project(NodeManagerUT CXX)

# Compilation options
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(cmake/flags.cmake)
include(cmake/dependencies.cmake)
include(cmake/ut.cmake)

option(COVERAGE "Generate code coverage information" ON)


set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCLOCK_MOCK")

if(COVERAGE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")

endif(COVERAGE)

add_executable (
    nm_tests
    src/main.cpp
    src/utils/dbus_environment.cpp
    )

add_dependencies (nm_tests sdbusplus-project)
add_dependencies (nm_tests phosphor-logging)
add_dependencies (nm_tests libpeci)
add_dependencies (nm_tests libgpiod)
add_dependencies (nm_tests boost-1-77)
add_dependencies (nm_tests googletest)
add_dependencies (nm_tests nlohmann-json)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(nm_tests
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
target_link_libraries (
    nm_tests
    systemd
    stdc++fs
    sdbusplus
    udev
    peci
    gtest
    gmock
    gpiodcxx
    )

set (UT_NM "nm_tests")
set (OBJECT_DIR ${CMAKE_BINARY_DIR}/CMakeFiles/${UT_NM}.dir)

include(../provisioning.settings)
configure_file(../cmake/config_defaults.hpp.in ../src/config_defaults.hpp @ONLY)

add_test(
    NAME ${UT_NM}
    COMMAND ${UT_NM}
)

add_custom_target(coverage
    COMMAND mkdir -p coverage
    #COMMAND ${CMAKE_MAKE_PROGRAM} test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)


# Filter coverage data only for files in the following path:
get_filename_component (SOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../include" ABSOLUTE)

add_custom_command(TARGET coverage
    COMMAND echo "Generating coverage report..."
    COMMAND lcov --capture --rc lcov_branch_coverage=1 --directory ${OBJECT_DIR} --output-file coverage.info
    COMMAND lcov --extract coverage.info '${SOURCES_PATH}/*' --output-file coverage.info
    COMMAND genhtml coverage.info --output-directory .
    COMMAND echo "Output generated in ${CMAKE_BINARY_DIR}/coverage"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/coverage
)
