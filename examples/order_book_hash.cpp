// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/stable_abi/stable_abi.hpp>

#include <cstddef>
#include <iostream>

namespace stable_abi = beman::stable_abi;

struct order {
    int side = 1;
    std::size_t quantity = 0;
};

template <typename T>
struct my_list {
    T start;
    T end;
    bool valid : 1 = false;
};

struct order_book {
    my_list<order> buy_orders;
    my_list<order> sell_orders;
};

int main() {
    std::cout
        << stable_abi::get_abi_hash<
               order_book,
               stable_abi::abi_hashing_config{.include_indirections = true}>()
        << '\n';
}
