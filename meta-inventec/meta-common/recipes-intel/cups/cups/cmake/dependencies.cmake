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

cmake_minimum_required (VERSION 3.5)

include (ExternalProject)

####### Boost #######
find_package (Boost 1.71 REQUIRED)
include_directories (SYSTEM ${BOOST_SRC_DIR})

# Verbose debugging of asio internals
# add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

# Main config options
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)
add_definitions (-DBOOST_BEAST_USE_STD_STRING_VIEW)
add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)

# Deprecation settings
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
message (BOOST_VERSION = ${Boost_VERSION})
if ("${Boost_VERSION}" STREQUAL "107100")
    add_definitions (-DBOOST_ASIO_NO_DEPRECATED)
endif ()

# ASIO uses deprecated coroutine library
add_definitions (-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
