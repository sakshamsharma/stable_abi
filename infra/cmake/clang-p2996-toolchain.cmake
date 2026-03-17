# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

include_guard(GLOBAL)

set(
    CMAKE_C_COMPILER
    "/Users/sakshamsharma/src/clang/reflection/build/bin/clang"
)
set(
    CMAKE_CXX_COMPILER
    "/Users/sakshamsharma/src/clang/reflection/build/bin/clang++"
)

set(
    BEMAN_STABLE_ABI_P2996_SYSROOT
    "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
)
set(
    BEMAN_STABLE_ABI_P2996_LIBCXX_INCLUDE
    "/Users/sakshamsharma/src/clang/reflection/build/include/c++/v1"
)
set(
    BEMAN_STABLE_ABI_P2996_LIB_DIR
    "/Users/sakshamsharma/src/clang/reflection/build/lib"
)

set(
    BEMAN_STABLE_ABI_P2996_CXX_FLAGS
    "-freflection -fexpansion-statements -isysroot ${BEMAN_STABLE_ABI_P2996_SYSROOT} -nostdinc++ -isystem ${BEMAN_STABLE_ABI_P2996_LIBCXX_INCLUDE} -stdlib=libc++"
)
set(
    BEMAN_STABLE_ABI_P2996_LINK_FLAGS
    "-L${BEMAN_STABLE_ABI_P2996_LIB_DIR} -Wl,-rpath,${BEMAN_STABLE_ABI_P2996_LIB_DIR} -stdlib=libc++"
)

set(CMAKE_C_FLAGS_INIT "-isysroot ${BEMAN_STABLE_ABI_P2996_SYSROOT}")
set(CMAKE_CXX_FLAGS_INIT "${BEMAN_STABLE_ABI_P2996_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${BEMAN_STABLE_ABI_P2996_LINK_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${BEMAN_STABLE_ABI_P2996_LINK_FLAGS}")

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}")
