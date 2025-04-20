cmake_minimum_required(VERSION 3.28)

project(crc
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A SIMD-accelerated C++ CRC library."
    HOMEPAGE_URL "https://github.com/LocalSpook/crc"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

add_library(crc INTERFACE)
add_library(crc::crc ALIAS crc)
target_sources(crc
    INTERFACE FILE_SET HEADERS BASE_DIRS include FILES
        include/crc/crc.hpp
)
target_compile_features(crc INTERFACE cxx_std_20)

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

if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2
        GIT_TAG v3.7.1
        SYSTEM
    )
    FetchContent_MakeAvailable(catch2)

    add_executable(tests)
    target_sources(tests PRIVATE test/tests.cpp)
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain crc::crc)
endif()
