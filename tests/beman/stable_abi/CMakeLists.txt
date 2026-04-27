# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

add_executable(beman.stable_abi.tests.abi_hash)
target_sources(
    beman.stable_abi.tests.abi_hash
    PRIVATE abi_hash.test.cpp
)
target_link_libraries(
    beman.stable_abi.tests.abi_hash
    PRIVATE beman::stable_abi
)

add_test(
    NAME beman.stable_abi.tests.abi_hash
    COMMAND $<TARGET_FILE:beman.stable_abi.tests.abi_hash>
)
