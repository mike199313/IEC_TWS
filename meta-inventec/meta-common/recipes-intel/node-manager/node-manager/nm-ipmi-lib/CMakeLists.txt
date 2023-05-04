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

project (NmIpmiLib CXX)

# Compilation options
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# General options
option (ENABLE_LOGGING_MECHANISM "Enables logging mechanism" ON)

# libipmid
find_package (PkgConfig REQUIRED)
pkg_check_modules (LIBIPMID libipmid REQUIRED)
include_directories (SYSTEM ${LIBIPMID_INCLUDE_DIRS})
link_directories (${LIBIPMID_LIBRARY_DIRS})

# Add location with header files
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define source files
set (SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/nm_commands.cpp)

# Library
add_library (zintelnmipmicmds SHARED ${SRC_FILES} ${TEST_FILE})

# Set library properties
target_compile_definitions(zintelnmipmicmds
                        PRIVATE
                        $<$<BOOL:${ENABLE_LOGGING_MECHANISM}>: -DENABLE_LOGGING>)

set_target_properties (zintelnmipmicmds PROPERTIES VERSION "0.1.0")
set_target_properties (zintelnmipmicmds PROPERTIES SOVERSION "0")

install (TARGETS zintelnmipmicmds DESTINATION lib/ipmid-providers)
