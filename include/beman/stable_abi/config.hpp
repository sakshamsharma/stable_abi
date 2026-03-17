// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_STABLE_ABI_CONFIG_HPP
#define BEMAN_STABLE_ABI_CONFIG_HPP

#include <cstdint>
#include <type_traits>

namespace beman::stable_abi::v0 {
using version = std::integral_constant<int, 0>;

struct abi_hashing_config {
    static constexpr int minimum_supported_version = 0;
    static constexpr int maximum_supported_version = 0;

    std::uint8_t version : 4 = 0;
    bool include_nsdm_names : 1 = true;
    bool include_indirections : 1 = false;
} __attribute__((packed));

enum class vtable_hashing_mode : std::uint8_t {
    none,
    signature,
    signature_and_names,
    num_vtable_hashing_modes
};

enum class virtual_abi : std::uint8_t { itanium, msvc };

struct indirection_abi_hashing_config {
    std::uint8_t version : 4 = 0;
    vtable_hashing_mode virtual_hashing_mode : 4 =
        vtable_hashing_mode::none;
    virtual_abi abi_mode : 2 = virtual_abi::itanium;
    bool recurse_references : 1 = true;
} __attribute__((packed));
}  // namespace beman::stable_abi::v0

namespace beman::stable_abi {
using version = v0::version;
using abi_hashing_config = v0::abi_hashing_config;
using indirection_abi_hashing_config = v0::indirection_abi_hashing_config;
using vtable_hashing_mode = v0::vtable_hashing_mode;
using virtual_abi = v0::virtual_abi;
}  // namespace beman::stable_abi

#endif
