# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.30...4.3)

project(
    beman.stable_abi
    DESCRIPTION "Stable ABI comparison utilities explored with static reflection"
    LANGUAGES CXX
    VERSION 0.1.0
)

option(
    BEMAN_STABLE_ABI_BUILD_TESTS
    "Enable building tests and test infrastructure. Default: OFF until the first implementation lands."
    OFF
)

option(
    BEMAN_STABLE_ABI_BUILD_EXAMPLES
    "Enable building examples. Default: OFF until the first implementation lands."
    OFF
)

include(infra/cmake/beman-install-library.cmake)

add_library(beman.stable_abi INTERFACE)
add_library(beman::stable_abi ALIAS beman.stable_abi)

target_sources(
    beman.stable_abi
    PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(
    beman.stable_abi
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ${PROJECT_IS_TOP_LEVEL}
)

add_subdirectory(include/beman/stable_abi)

beman_install_library(beman.stable_abi TARGETS beman.stable_abi)

if(BEMAN_STABLE_ABI_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
    enable_testing()
    add_subdirectory(tests/beman/stable_abi)
endif()

if(BEMAN_STABLE_ABI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
