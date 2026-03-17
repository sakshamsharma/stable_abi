# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

add_executable(beman.stable_abi.examples.order_book_hash)
target_sources(
    beman.stable_abi.examples.order_book_hash
    PRIVATE order_book_hash.cpp
)
target_link_libraries(
    beman.stable_abi.examples.order_book_hash
    PRIVATE beman::stable_abi
)
