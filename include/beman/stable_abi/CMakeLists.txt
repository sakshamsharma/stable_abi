# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

target_sources(
    beman.stable_abi
    PUBLIC
        FILE_SET HEADERS
            FILES
                abi_description.hpp
                abi_hash.hpp
                config.hpp
                crc32.hpp
                stable_abi.hpp
)
