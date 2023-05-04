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
cmake_policy (SET CMP0054 NEW)

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

project (peci-cups CXX)

# General options
option (ENABLE_UT "Enable unit tests" OFF)
option (ENABLE_UT_EXECUTION "Enable UT execution as part of build process" OFF)
option (UT_ONLY "Compile only unit tests" OFF)
option (ENABLE_LOGS "Enable logging" ON)
option (ENABLE_PECI_TRACE "Enable PECI payload tracing" OFF)

# Compilation options
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Yocto integration
option(YOCTO_DEPENDENCIES "Use YOCTO dependencies system" OFF)

# Compilation flags
include(cmake/flags.cmake)

# Build unit tests
if (ENABLE_UT)
    include(cmake/ut.cmake)
    add_subdirectory(tests)
endif (ENABLE_UT)

# Include directories
include_directories (src)
include_directories (src/utils)

# Build dependencies
include(cmake/dependencies.cmake)

# Source files
set (
    SRC_FILES
    src/peci/transport/adapter.cpp
    src/base/loadFactors.cpp
    src/main.cpp
)

# Application
if(UT_ONLY)
    MESSAGE(INFO "Compiling only unit tests as UT_ONLY option is provided")
else()
    add_executable (cups-service ${SRC_FILES})

    target_link_libraries (cups-service peci)
    target_link_libraries (cups-service systemd)
    target_link_libraries (cups-service sdbusplus)
    target_link_libraries (cups-service boost_coroutine)

    target_compile_definitions (
        cups-service
        PRIVATE
        $<$<BOOL:${ENABLE_LOGS}>: -DENABLE_LOGS>
        $<$<BOOL:${ENABLE_PECI_TRACE}>: -DENABLE_PECI_TRACE>
        )

    install (
        TARGETS cups-service
        DESTINATION sbin)

    install(
        FILES ${PROJECT_SOURCE_DIR}/xyz.openbmc_project.CupsService.service
        DESTINATION /lib/systemd/system/)
endif (UT_ONLY)
