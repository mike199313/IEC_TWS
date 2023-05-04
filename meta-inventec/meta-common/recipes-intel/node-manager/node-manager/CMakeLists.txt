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

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
cmake_policy(SET CMP0054 NEW)

project(NodeManager CXX)

# Compilation options
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

# General options
option (YOCTO_DEPENDENCIES "Use YOCTO dependencies system" OFF)
option (ENABLE_NM_IPMI_CMDS "Enable IPMI commands" ON)
option (ENABLE_NM "Enable Node Manager app compilation" ON)
option (ENABLE_PECI "Enables PECI commands execution, if OFF will always return a timeout status immediately when command issued" ON)

# Include compilation flags
include(cmake/flags.cmake)

# Include externals
include(cmake/dependencies.cmake)

# Compile Node Manger IPMI commands library
if (${ENABLE_NM_IPMI_CMDS})
    add_subdirectory(nm-ipmi-lib)
endif (${ENABLE_NM_IPMI_CMDS})

# Compile Node Manager app
if (${ENABLE_NM})
    include(provisioning.settings)
    configure_file(cmake/config_defaults.hpp.in config_defaults.hpp @ONLY)
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    # Add location with header files
    include_directories ("include")

    # Define source files
    set(SRC_FILES src/main.cpp)

    # Executables
    add_executable(node-manager ${SRC_FILES} ${HEADER_FILES})

    # Default linkage
    set(COMMON_LIBS systemd sdbusplus boost_coroutine boost_context peci gpiodcxx)
    target_link_libraries(node-manager ${COMMON_LIBS})

    install(TARGETS node-manager DESTINATION sbin)

    # Options based compile definitions
    target_compile_definitions(node-manager
                           PRIVATE
                           $<$<BOOL:${ENABLE_PECI}>: -DENABLE_PECI>
                           $<$<BOOL:${CUSTOM_DBUS_PATH}>:
                           -DCUSTOM_DBUS_PATH="${CUSTOM_DBUS_PATH}">)

    install(FILES ${PROJECT_SOURCE_DIR}/xyz.openbmc_project.NodeManager.service
                  DESTINATION /lib/systemd/system/)
endif (${ENABLE_NM})
