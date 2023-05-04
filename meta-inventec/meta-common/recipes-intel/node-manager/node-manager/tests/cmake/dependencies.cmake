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

# Download and unpack external dependencies
include (ExternalProject)

####### sdbusplus #######
externalproject_add (
    sdbusplus-project
    PREFIX "${CMAKE_BINARY_DIR}/sdbusplus-project"
    GIT_REPOSITORY "https://github.com/openbmc/sdbusplus.git"
    GIT_TAG 11a50dda6e05081418b78333e2e42f8d896e7b45
    SOURCE_DIR "${CMAKE_BINARY_DIR}/sdbusplus-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/sdbusplus-build"
    CONFIGURE_COMMAND cd "${CMAKE_BINARY_DIR}/sdbusplus-src" && meson -Dtests=disabled -Dexamples=disabled build
    BUILD_COMMAND cd "${CMAKE_BINARY_DIR}/sdbusplus-src/build" && ninja
    INSTALL_COMMAND cd "${CMAKE_BINARY_DIR}/sdbusplus-src/tools" && ./setup.py build
    UPDATE_COMMAND ""
    LOG_DOWNLOAD ON
)
include_directories (${CMAKE_BINARY_DIR}/sdbusplus-src/include/)
link_directories (${CMAKE_BINARY_DIR}/sdbusplus-src/build)

####### phosphor-logging #######
externalproject_add (
    phosphor-logging
    PREFIX "${CMAKE_BINARY_DIR}/phosphor-logging"
    GIT_REPOSITORY "https://github.com/openbmc/phosphor-logging.git"
    GIT_TAG 3477ce94b366e6968296377598122a2060f4bed2
    SOURCE_DIR "${CMAKE_BINARY_DIR}/phosphor-logging-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/phosphor-logging-build"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
include_directories (${CMAKE_BINARY_DIR}/phosphor-logging-src)

####### libpeci #######
externalproject_add (
    libpeci
    PREFIX "${CMAKE_BINARY_DIR}/libpeci"
    GIT_REPOSITORY "https://github.com/openbmc/libpeci.git"
    GIT_TAG bc641112abc99b4a972665aa984023a6713a21ac
    SOURCE_DIR "${CMAKE_BINARY_DIR}/libpeci-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/libpeci-build"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cd "${CMAKE_BINARY_DIR}/libpeci-build" && cmake "${CMAKE_BINARY_DIR}/libpeci-src" && make
    INSTALL_COMMAND ""
)
include_directories (SYSTEM ${CMAKE_BINARY_DIR}/libpeci-src)
link_directories (${CMAKE_BINARY_DIR}/libpeci-build/)

####### gpiod #######
externalproject_add (
    libgpiod
    PREFIX "${CMAKE_BINARY_DIR}/libgpiod"
    GIT_REPOSITORY "https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git"
    GIT_TAG bb4e5ce7071feed41bd3f0d9a62b5033fd483a18
    SOURCE_DIR "${CMAKE_BINARY_DIR}/libgpiod-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/libgpiod-build"
    CONFIGURE_COMMAND cd "${CMAKE_BINARY_DIR}/libgpiod-src" && ${CMAKE_BINARY_DIR}/libgpiod-src/autogen.sh --enable-bindings-cxx --prefix=${CMAKE_BINARY_DIR}/libgpiod-build
    BUILD_COMMAND cd "${CMAKE_BINARY_DIR}/libgpiod-src" && make
    INSTALL_COMMAND cd "${CMAKE_BINARY_DIR}/libgpiod-src" && make install
)
include_directories (SYSTEM ${CMAKE_BINARY_DIR}/libgpiod-build/include)
link_directories (${CMAKE_BINARY_DIR}/libgpiod-build/lib)

####### gtest #######
externalproject_add (
    googletest
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG 58d77fa8070e8cec2dc1ed015d66b454c8d78850
    SOURCE_DIR "${CMAKE_BINARY_DIR}/googletest-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/googletest-build"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dependencies
)
include_directories (${CMAKE_BINARY_DIR}/googletest-src/googletest/include)
include_directories (${CMAKE_BINARY_DIR}/googletest-src/googlemock/include)
link_directories (${CMAKE_BINARY_DIR}/googletest-build/lib)

####### nlohmann-json-v3.9.1 #######
externalproject_add (
    nlohmann-json
    GIT_REPOSITORY "https://github.com/nlohmann/json.git"
    GIT_TAG db78ac1d7716f56fc9f1b030b715f872f93964e4
    SOURCE_DIR "${CMAKE_BINARY_DIR}/nlohmann-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/nlohmann-build"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
include_directories (${CMAKE_BINARY_DIR}/nlohmann-src/include)

####### boost-1-77 #######
externalproject_add (
    boost-1-77
    URL https://boostorg.jfrog.io/artifactory/main/release/1.77.0/source/boost_1_77_0.tar.gz
    URL_HASH SHA256=5347464af5b14ac54bb945dc68f1dd7c56f0dad7262816b956138fc53bcc0131
    SOURCE_DIR "${CMAKE_BINARY_DIR}/boost-src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/boost-build"
    CONFIGURE_COMMAND cd ${CMAKE_BINARY_DIR}/boost-src &&
                      ./bootstrap.sh --with-libraries=coroutine,context
                                     --prefix=${CMAKE_BINARY_DIR}/dependencies
    BUILD_COMMAND cd ${CMAKE_BINARY_DIR}/boost-src && ./b2
    INSTALL_COMMAND cd ${CMAKE_BINARY_DIR}/boost-src && ./b2 -d0 install
)
include_directories (${CMAKE_BINARY_DIR}/boost-src)
link_directories (${CMAKE_BINARY_DIR}/boost-src/libs)

add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DBOOST_NO_RTTI)
add_definitions(-DBOOST_NO_TYPEID)
add_definitions(-DBOOST_ASIO_DISABLE_THREADS)
add_definitions(-DBOOST_ASIO_DISABLE_THREADS)
add_definitions(-DBOOST_BEAST_USE_STD_STRING_VIEW)
