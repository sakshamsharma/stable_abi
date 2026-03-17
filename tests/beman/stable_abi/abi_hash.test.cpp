// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/stable_abi/stable_abi.hpp>

#include <gtest/gtest.h>

#include <cstddef>

namespace stable_abi = beman::stable_abi;

namespace {
struct named_member_a {
    int left = 0;
};

struct named_member_b {
    int right = 0;
};

struct layout_a {
    int side = 0;
    std::size_t quantity = 0;
};

struct layout_b {
    std::size_t quantity = 0;
    int side = 0;
};

template <typename T>
struct list_like {
    T start;
    T end;
    bool valid : 1 = false;
};

struct order_book {
    list_like<layout_a> buy_orders;
    list_like<layout_a> sell_orders;
};
}  // namespace

TEST(abi_hash, member_names_can_be_ignored) {
    constexpr auto lhs =
        stable_abi::get_abi_hash<named_member_a,
                                 stable_abi::abi_hashing_config{
                                     .include_nsdm_names = false}>();
    constexpr auto rhs =
        stable_abi::get_abi_hash<named_member_b,
                                 stable_abi::abi_hashing_config{
                                     .include_nsdm_names = false}>();
    EXPECT_EQ(lhs, rhs);
}

TEST(abi_hash, layout_changes_affect_the_hash) {
    constexpr auto lhs = stable_abi::get_abi_hash<layout_a>();
    constexpr auto rhs = stable_abi::get_abi_hash<layout_b>();
    EXPECT_NE(lhs, rhs);
}

TEST(abi_hash, nested_layouts_hash_recursively) {
    constexpr auto flat = stable_abi::get_abi_hash<layout_a>();
    constexpr auto nested = stable_abi::get_abi_hash<order_book>();
    EXPECT_NE(flat, nested);
}
