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

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} \
    -Wall \
    -flto \
    -Os \
")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -Wall \
    -Wextra \
    -Wnon-virtual-dtor \
    -Wold-style-cast \
    -Wcast-align \
    -Wunused \
    -Woverloaded-virtual \
    -Wpedantic \
    -Wconversion \
    -Wsign-conversion \
    -Wnull-dereference \
    -Wdouble-promotion \
    -Wformat=2 \
    -Wno-unused-parameter \
")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0)
        set (
            CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} \
            -Werror \
            -Wduplicated-cond \
            -Wduplicated-branches \
            -Wlogical-op \
            -Wnull-dereference \
            -Wdouble-promotion \
            -Wformat=2 \
            -Wno-unused-parameter \
        ")
    endif (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0)
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

# Security-oriented compilation flags
set (SECURITY_FLAGS
    "-fstack-protector-strong \
    -fPIE \
    -fPIC \
    -D_FORTIFY_SOURCE=2 \
    -Wformat \
    -Wformat-security \
")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} \
    ${SECURITY_FLAGS}")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} \
    ${SECURITY_FLAGS}")
set (CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} \
    ${SECURITY_FLAGS}")
