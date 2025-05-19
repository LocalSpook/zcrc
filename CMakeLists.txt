cmake_minimum_required(VERSION 3.28.2)

project(crc
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A SIMD-accelerated C++ CRC library."
    HOMEPAGE_URL "https://github.com/LocalSpook/crc"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

include(FetchContent)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(CRC_MODULE "build the library as a module" OFF)
option(CRC_TEST "build the tests" OFF)
option(CRC_BENCHMARK "build the benchmarks" OFF)

add_library(crc INTERFACE)
add_library(crc::crc ALIAS crc)
target_sources(crc
    INTERFACE FILE_SET HEADERS BASE_DIRS include FILES
        include/crc/crc.hpp
)
target_compile_features(crc INTERFACE cxx_std_20)

if(CRC_MODULE)
    add_library(crc-module)
    add_library(crc::crc-module ALIAS crc-module)
    target_sources(crc-module
        PUBLIC FILE_SET CXX_MODULES BASE_DIRS src FILES
            src/crc.cppm
    )
    target_link_libraries(crc-module PUBLIC crc)
    target_compile_features(crc-module PUBLIC cxx_std_20)
endif()

install(
    TARGETS crc
    EXPORT crc-targets
    PUBLIC_HEADER
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/crc
        FILE_SET HEADERS
)

write_basic_package_version_file(crc-config-version.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/crc-config.cmake [[
include(${CMAKE_CURRENT_LIST_DIR}/crc-targets.cmake)
]])

install(
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/crc-config.cmake
		${CMAKE_CURRENT_BINARY_DIR}/crc-config-version.cmake
    DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/crc
)

install(
	EXPORT crc-targets
	FILE crc-targets.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/crc
	NAMESPACE crc::
)

if(CRC_TEST OR CRC_BECNHMARK)
    FetchContent_Declare(catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2
        GIT_TAG v3.7.1
        SYSTEM
    )
    FetchContent_MakeAvailable(catch2)
endif()

if(CRC_TEST)
    add_executable(tests)
    target_sources(tests PRIVATE test/tests.cpp)
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain crc::crc)
    if(CRC_MODULE)
        target_compile_definitions(tests PRIVATE CRC_TEST_MODULE)
        target_link_libraries(tests PRIVATE crc::crc-module)
    endif()
endif()

if(CRC_BENCHMARK)
    if(NOT CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
        message(WARNING "Building without optimization; benchmark results will be bogus! (CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}, but should be Release, RelWithDebInfo, or MinSizeRel)")
    endif()

    add_executable(benchmarks)
    target_sources(benchmarks PRIVATE benchmark/benchmarks.cpp)
    target_link_libraries(benchmarks PRIVATE Catch2::Catch2WithMain crc::crc)
endif()
