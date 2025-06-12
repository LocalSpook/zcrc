cmake_minimum_required(VERSION 3.28.2)

project(zcrc
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A high-performance C++ CRC library."
    HOMEPAGE_URL "https://github.com/LocalSpook/zcrc"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

include(FetchContent)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(ZCRC_MODULE "build the library as a module" OFF)
option(ZCRC_TEST "build the tests" OFF)
option(ZCRC_BENCHMARK "build the benchmarks" OFF)

add_library(zcrc INTERFACE)
add_library(zcrc::zcrc ALIAS zcrc)
target_sources(zcrc
    INTERFACE FILE_SET HEADERS BASE_DIRS include FILES
        include/zcrc/zcrc.hpp
)
target_compile_features(zcrc INTERFACE cxx_std_20)

if(ZCRC_MODULE)
    add_library(zcrc-module)
    add_library(zcrc::zcrc-module ALIAS zcrc-module)
    target_sources(zcrc-module
        PUBLIC FILE_SET CXX_MODULES BASE_DIRS src FILES
            src/zcrc.cppm
    )
    target_link_libraries(zcrc-module PUBLIC zcrc)
    target_compile_features(zcrc-module PUBLIC cxx_std_20)
endif()

install(
    TARGETS zcrc
    EXPORT zcrc-targets
    PUBLIC_HEADER
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zcrc
        FILE_SET HEADERS
)

write_basic_package_version_file(zcrc-config-version.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/zcrc-config.cmake [[
include(${CMAKE_CURRENT_LIST_DIR}/zcrc-targets.cmake)
]])

install(
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/zcrc-config.cmake
		${CMAKE_CURRENT_BINARY_DIR}/zcrc-config-version.cmake
    DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/zcrc
)

install(
	EXPORT zcrc-targets
	FILE zcrc-targets.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zcrc
	NAMESPACE zcrc::
)

if(ZCRC_TEST OR ZCRC_BENCHMARK)
    FetchContent_Declare(catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2
        GIT_TAG v3.7.1
        SYSTEM
    )
    FetchContent_MakeAvailable(catch2)
endif()

# Workaround: CMake since 3.28 will, by default, scan *all* C++ files compiled as
# C++20 or later for module dependencies using the clang-scan-deps tool. Problem
# is, the Emscripten SDK before 3.1.57 bundles an LLVM without clang-scan-deps,
# causing build failures even when not using modules, so we manually tell CMake
# to skip scanning targets that don't need modules.
#
# https://github.com/emscripten-core/emscripten/issues/22305
function(_zcrc_disable_module_dependency_scanning target)
    set_target_properties(${target} PROPERTIES CXX_SCAN_FOR_MODULES OFF)
endfunction()

if(ZCRC_TEST)
    add_executable(tests)
    target_sources(tests PRIVATE test/tests.cpp)
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain zcrc::zcrc)
    _zcrc_disable_module_dependency_scanning(tests)
    if(ZCRC_MODULE)
        add_library(module-tests OBJECT)
        target_sources(module-tests PRIVATE test/tests.cpp)
        target_compile_definitions(module-tests PRIVATE ZCRC_TEST_MODULE)
        target_link_libraries(module-tests PRIVATE Catch2::Catch2 zcrc::zcrc-module)
        target_link_libraries(tests PRIVATE module-tests)
    endif()
endif()

if(ZCRC_BENCHMARK)
    if(NOT CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
        message(WARNING "Building without optimization; benchmark results will be bogus! (CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}, but should be Release, RelWithDebInfo, or MinSizeRel)")
    endif()

    add_executable(benchmarks)
    target_sources(benchmarks PRIVATE benchmark/benchmarks.cpp)
    target_link_libraries(benchmarks PRIVATE Catch2::Catch2WithMain zcrc::zcrc)
    _zcrc_disable_module_dependency_scanning(benchmarks)
endif()
