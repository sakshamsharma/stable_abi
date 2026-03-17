// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_STABLE_ABI_ABI_HASH_HPP
#define BEMAN_STABLE_ABI_ABI_HASH_HPP

#include <beman/stable_abi/config.hpp>
#include <beman/stable_abi/crc32.hpp>

#if !__has_include(<meta>)
#error "beman.stable_abi currently requires a standard library that provides <meta>."
#endif

#include <cstddef>
#include <functional>
#include <meta>
#include <string_view>

namespace beman::stable_abi {
namespace meta = std::meta;

template <typename T, abi_hashing_config Config = abi_hashing_config{},
          indirection_abi_hashing_config IndirectionConfig =
              indirection_abi_hashing_config{}>
struct abi_hash {};

template <typename T, abi_hashing_config Config = abi_hashing_config{},
          indirection_abi_hashing_config IndirectionConfig =
              indirection_abi_hashing_config{}>
consteval std::size_t get_abi_hash();

namespace detail {
consteval std::size_t hash_string(std::string_view value) {
    return crc32(value);
}

consteval std::size_t hash_info_name(std::size_t seed, meta::info reflection) {
    if (meta::has_identifier(reflection)) {
        return hash_combine(seed, hash_string(meta::identifier_of(reflection)));
    }
    return hash_combine(seed, hash_string(meta::display_string_of(reflection)));
}

consteval std::size_t hash_member_layout(std::size_t seed,
                                         meta::info member) {
    const auto offset = meta::offset_of(member);
    seed = hash_combine(seed, static_cast<std::size_t>(offset.bytes));
    seed = hash_combine(seed, static_cast<std::size_t>(offset.bits));
    seed = hash_combine(seed, meta::bit_size_of(member));
    return seed;
}

}  // namespace detail

template <typename T, abi_hashing_config Config,
          indirection_abi_hashing_config IndirectionConfig>
consteval std::size_t get_abi_hash() {
    constexpr auto context = meta::access_context::unchecked();

    std::size_t hash = 0;
    template for (constexpr auto member :
                  std::define_static_array(
                      meta::nonstatic_data_members_of(^^T, context))) {
        constexpr auto member_type = meta::type_of(member);
        constexpr bool is_pointer = meta::is_pointer_type(member_type);
        constexpr bool is_reference = meta::is_reference_type(member_type);
        constexpr bool is_indirection = is_pointer || is_reference;

        if constexpr (!Config.include_indirections && is_indirection) {
            continue;
        }

        if constexpr (Config.include_nsdm_names) {
            hash = detail::hash_info_name(hash, member);
        }

        hash = detail::hash_member_layout(hash, member);

        if constexpr (meta::is_class_type(member_type)) {
            hash = detail::hash_combine(
                hash,
                get_abi_hash<typename [:member_type:], Config,
                             IndirectionConfig>());
        } else if constexpr (Config.include_indirections &&
                             IndirectionConfig.recurse_references &&
                             is_pointer) {
            constexpr auto pointee = meta::remove_pointer(member_type);
            if constexpr (meta::is_class_type(pointee)) {
                hash = detail::hash_combine(
                    hash,
                    get_abi_hash<typename [:pointee:], Config,
                                 IndirectionConfig>());
            } else {
                hash = detail::hash_combine(
                    hash, detail::hash_string(meta::display_string_of(pointee)));
            }
        } else if constexpr (Config.include_indirections &&
                             IndirectionConfig.recurse_references &&
                             is_reference) {
            constexpr auto referent = meta::remove_reference(member_type);
            if constexpr (meta::is_class_type(referent)) {
                hash = detail::hash_combine(
                    hash,
                    get_abi_hash<typename [:referent:], Config,
                                 IndirectionConfig>());
            } else {
                hash = detail::hash_combine(
                    hash,
                    detail::hash_string(meta::display_string_of(referent)));
            }
        } else {
            hash = detail::hash_combine(
                hash, detail::hash_string(meta::display_string_of(member_type)));
        }
    };

    if constexpr (Config.include_indirections &&
                  IndirectionConfig.virtual_hashing_mode !=
                      vtable_hashing_mode::none) {
        // The original prototype left virtual ABI hashing as future work.
    }

    hash = detail::hash_combine(hash, meta::size_of(^^T));
    return hash;
}
}  // namespace beman::stable_abi

namespace std {
template <typename T, beman::stable_abi::abi_hashing_config Config,
          beman::stable_abi::indirection_abi_hashing_config IndirectionConfig>
struct hash<beman::stable_abi::abi_hash<T, Config, IndirectionConfig>> {
    size_t operator()() const {
        return beman::stable_abi::get_abi_hash<T, Config, IndirectionConfig>();
    }
};
}  // namespace std

#endif
