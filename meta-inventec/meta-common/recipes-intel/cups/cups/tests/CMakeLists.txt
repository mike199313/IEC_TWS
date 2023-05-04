cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

add_executable (cups-ut
    peci/metrics/average.cpp
    peci/metrics/core.cpp
    peci/metrics/iio.cpp
    peci/metrics/impl.cpp
    peci/metrics/memory.cpp
    peci/mocks.cpp
    peci/transport/abi.cpp
    peci/transport/adapter.cpp
    peci/abi.cpp
)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../src/utils)
include_directories (${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(cups-ut
    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

set (CUPS_UT "cups-ut")

add_test(
    NAME ${CUPS_UT}
    COMMAND ${CUPS_UT}
)

if (${ENABLE_UT_EXECUTION})
    add_custom_command(
        TARGET ${CUPS_UT}
        COMMENT "Run tests for CUPS components"
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
        COMMAND GTEST_COLOR=1 ${CMAKE_CTEST_COMMAND} --output-on-failure
    )
endif()

install (
        TARGETS cups-ut
        DESTINATION bin
)
