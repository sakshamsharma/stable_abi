// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_STABLE_ABI_ABI_HASH_HPP
#define BEMAN_STABLE_ABI_ABI_HASH_HPP

#include <beman/stable_abi/config.hpp>
#include <beman/stable_abi/crc32.hpp>

#if !__has_include(<experimental/meta>)
#error "beman.stable_abi currently requires a compiler that provides <experimental/meta>."
#endif

#include <cstddef>
#include <experimental/meta>
#include <functional>
#include <string_view>
#include <type_traits>

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
template <auto Reflected>
consteval std::size_t hash_entity_name(std::size_t seed) {
    return hash_combine(seed, crc32(std::string_view(meta::name_of(Reflected))));
}

template <auto ReflectedMember>
consteval std::size_t hash_member_layout(std::size_t seed) {
    if constexpr (meta::is_bit_field(ReflectedMember)) {
        seed = hash_combine(seed, meta::offset_of(ReflectedMember));
        seed = hash_combine(seed, meta::bit_offset_of(ReflectedMember));
        seed = hash_combine(seed, meta::bit_size_of(ReflectedMember));
    } else {
        seed = hash_combine(seed, meta::offset_of(ReflectedMember));
        seed = hash_combine(seed, meta::size_of(ReflectedMember));
    }
    return seed;
}

template <typename T, abi_hashing_config Config,
          indirection_abi_hashing_config IndirectionConfig>
consteval std::size_t get_type_component_hash() {
    if constexpr (meta::test_type(^std::is_class_v, ^T)) {
        return get_abi_hash<T, Config, IndirectionConfig>();
    } else {
        return crc32(std::string_view(meta::name_of(^T)));
    }
}
}  // namespace detail

consteval std::size_t get_abi_hash() {
    std::size_t hash = 0;

    [: expand(meta::nonstatic_data_members_of(^T)) :] >> [&]<auto Member> {
        constexpr auto member_type = meta::type_of(Member);
        constexpr bool is_pointer =
            meta::test_type(^std::is_pointer_v, member_type);
        constexpr bool is_reference =
            meta::test_type(^std::is_reference_v, member_type);
        constexpr bool is_indirection = is_pointer || is_reference;

        if constexpr (!Config.include_indirections && is_indirection) {
            return;
        }

        if constexpr (Config.include_nsdm_names) {
            hash = detail::hash_entity_name<Member>(hash);
        }

        hash = detail::hash_member_layout<Member>(hash);

        if constexpr (meta::test_type(^std::is_class_v, member_type)) {
            using nested_type = typename [:member_type:];
            hash = detail::hash_combine(
                hash, get_abi_hash<nested_type, Config, IndirectionConfig>());
        } else if constexpr (Config.include_indirections &&
                             IndirectionConfig.recurse_references &&
                             is_pointer) {
            constexpr auto pointee =
                meta::substitute(^std::remove_pointer, {member_type});
            using pointee_type = typename [:pointee:];
            hash = detail::hash_combine(
                hash, detail::get_type_component_hash<pointee_type, Config,
                                                      IndirectionConfig>());
        } else if constexpr (Config.include_indirections &&
                             IndirectionConfig.recurse_references &&
                             is_reference) {
            constexpr auto referent =
                meta::substitute(^std::remove_reference, {member_type});
            using referent_type = typename [:referent:];
            hash = detail::hash_combine(
                hash, detail::get_type_component_hash<referent_type, Config,
                                                      IndirectionConfig>());
        } else {
            using value_type = typename [:member_type:];
            hash = detail::hash_combine(
                hash, detail::get_type_component_hash<value_type, Config,
                                                      IndirectionConfig>());
        }
    };

    if constexpr (Config.include_indirections &&
                  IndirectionConfig.virtual_hashing_mode !=
                      vtable_hashing_mode::none) {
        // The original prototype left virtual ABI hashing as future work.
    }

    hash = detail::hash_combine(hash, sizeof(T));
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
