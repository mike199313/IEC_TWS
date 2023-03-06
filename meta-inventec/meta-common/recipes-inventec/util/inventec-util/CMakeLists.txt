cmake_minimum_required (VERSION 2.8.10 FATAL_ERROR)
set (BUILD_SHARED_LIBRARIES OFF)
include (ExternalProject)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -lstdc++fs \
    -Werror \
    -Wall \
    -Wextra \
    -Wnon-virtual-dtor \
    -Wold-style-cast \
    -Wcast-align \
    -Wunused \
    -Woverloaded-virtual \
    -Wpedantic \
    -Wmisleading-indentation \
    -Wduplicated-cond \
    -Wduplicated-branches \
    -Wlogical-op \
    -Wnull-dereference \
    -Wuseless-cast \
    -Wdouble-promotion \
    -Wformat=2 \
    -Wno-sign-compare \
    -Wno-reorder \
"
)
# todo: get rid of nos, add the below:
#  -Wshadow \
#  -Wconversion \

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set (PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config")

project (inventec_util CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDBUSPLUSPLUS sdbusplus REQUIRED)
include_directories(${SDBUSPLUSPLUS_INCLUDE_DIRS})
link_directories(${SDBUSPLUSPLUS_LIBRARY_DIRS})

set (MAC_UTIL_SRC_FILES src/mac_util.cpp )

set (MAC_UTIL_LINK_LIBS -lsystemd stdc++fs sdbusplus)

link_directories (${EXTERNAL_INSTALL_LOCATION}/lib)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable (mac_util ${MAC_UTIL_SRC_FILES})

target_link_libraries(mac_util ${MAC_UTIL_LINK_LIBS})

install (TARGETS mac_util DESTINATION bin)

install (PROGRAMS mac_util_config.sh DESTINATION bin)

set (EEPROM_UTIL_SRC_FILES src/eeprom_util.cpp )

set (EEPROM_UTIL_LINK_LIBS -lsystemd stdc++fs sdbusplus)

add_executable (eeprom_util ${EEPROM_UTIL_SRC_FILES})

target_link_libraries(eeprom_util ${EEPROM_UTIL_LINK_LIBS})

install (TARGETS eeprom_util DESTINATION bin)

install (PROGRAMS post_bootup.sh DESTINATION bin)

install (PROGRAMS pre_shutdown.sh DESTINATION bin)
