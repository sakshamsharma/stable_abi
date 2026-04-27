// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/stable_abi/stable_abi.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <span>

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

enum class mode : std::uint16_t { alpha = 1, beta = 7 };

struct point {
    std::int64_t x;
    double y;
};

struct with_array {
    std::int32_t counts[4];
    double scale;
};

struct non_trivial {
    non_trivial() = default;
    ~non_trivial() {}
    int value = 0;
};

double unary(double value) noexcept { return value; }
double unary_alias(double value) noexcept { return value + 1.0; }

point fold(point lhs, point rhs) noexcept {
    return point{lhs.x + rhs.x, lhs.y + rhs.y};
}

double sum_view(std::span<const double> values) noexcept {
    double total = 0.0;
    for (double value : values) {
        total += value;
    }
    return total;
}

std::span<const double> invalid_span_result(std::span<const double> values) noexcept {
    return values;
}

double may_throw(double value) { return value; }

double variadic_sum(int count, ...) noexcept { return static_cast<double>(count); }

non_trivial invalid_non_trivial_arg(non_trivial value) noexcept { return value; }

const stable_abi::type_description* find_type(const stable_abi::abi_spec_table& table,
                                              std::string_view id) {
    for (const auto& type : table.types) {
        if (type.id == id) {
            return &type;
        }
    }
    return nullptr;
}

const stable_abi::function_description* root_function(
    const stable_abi::abi_spec_table& table) {
    for (const auto& function : table.functions) {
        if (function.id == table.root_function_id) {
            return &function;
        }
    }
    return nullptr;
}
}  // namespace

static_assert(
    stable_abi::get_abi_hash<
        named_member_a,
        stable_abi::abi_hashing_config{.include_nsdm_names = false}>() ==
    stable_abi::get_abi_hash<
        named_member_b,
        stable_abi::abi_hashing_config{.include_nsdm_names = false}>());

static_assert(
    stable_abi::get_abi_hash<layout_a>() != stable_abi::get_abi_hash<layout_b>());

static_assert(
    stable_abi::get_abi_hash<layout_a>() !=
    stable_abi::get_abi_hash<order_book>());

int main() {
    {
        const auto table = stable_abi::describe_boundary_type<point>();
        const auto* root = find_type(table, table.root_type_id);
        assert(root != nullptr);
        assert(root->kind == stable_abi::type_kind::record);
        assert(root->boundary_transportable);
        assert(root->boundary_transport_kind ==
               stable_abi::transport_kind::by_value_record);
        assert(root->fields.size() == 2);
    }

    {
        const auto table = stable_abi::describe_boundary_type<with_array>();
        const auto* root = find_type(table, table.root_type_id);
        assert(root != nullptr);
        assert(root->boundary_transportable);
        assert(root->fields.size() == 2);
        const auto* array_type = find_type(table, root->fields[0].type_id);
        assert(array_type != nullptr);
        assert(array_type->kind == stable_abi::type_kind::array);
        assert(!array_type->boundary_transportable);
    }

    {
        const auto table =
            stable_abi::describe_native_type<std::span<const double>>();
        const auto* root = find_type(table, table.root_type_id);
        assert(root != nullptr);
        assert(table.mode == stable_abi::description_mode::native);
        assert(root->kind == stable_abi::type_kind::record);
    }

    {
        const auto table = stable_abi::describe_boundary_type<mode>();
        const auto* root = find_type(table, table.root_type_id);
        assert(root != nullptr);
        assert(root->kind == stable_abi::type_kind::enumeration);
        assert(root->enumerators.size() == 2);
        assert(root->boundary_transportable);
    }

    {
        const auto table = stable_abi::describe_function_signature<double(double) noexcept>();
        const auto* function = root_function(table);
        assert(function != nullptr);
        assert(function->boundary_callable);
        assert(function->signature_id.rfind("fnsig:", 0) == 0);
        assert(function->id.rfind("fn:", 0) == 0);
        assert(function->params.size() == 1);
        assert(function->params[0].passing == stable_abi::passing_kind::by_value);
        assert(function->result.passing == stable_abi::passing_kind::by_value);
    }

    {
        const auto left = stable_abi::describe_function<^^unary>();
        const auto right = stable_abi::describe_function<^^unary_alias>();
        const auto* left_fn = root_function(left);
        const auto* right_fn = root_function(right);
        assert(left_fn != nullptr);
        assert(right_fn != nullptr);
        assert(left_fn->id != right_fn->id);
        assert(left_fn->signature_id == right_fn->signature_id);
    }

    {
        const auto table = stable_abi::describe_function_signature<
            double(std::span<const double>) noexcept>();
        const auto* function = root_function(table);
        assert(function != nullptr);
        assert(function->boundary_callable);
        assert(function->params.size() == 1);
        assert(function->params[0].passing ==
               stable_abi::passing_kind::borrowed_view);
    }

    {
        const auto table = stable_abi::describe_function<^^sum_view>();
        const auto* function = root_function(table);
        assert(function != nullptr);
        assert(function->display_name == "sum_view");
        assert(function->boundary_callable);
        assert(function->params.size() == 1);
    }

    {
        const auto table = stable_abi::describe_function<^^invalid_span_result>();
        const auto* function = root_function(table);
        assert(function != nullptr);
        assert(!function->boundary_callable);
        assert(function->boundary_diagnostics.size() == 1);
        assert(function->boundary_diagnostics[0] ==
               "span<T> results are not boundary-callable in v1");
    }

    {
        const auto table = stable_abi::describe_function<^^may_throw>();
        const auto* function = root_function(table);
        assert(function != nullptr);
        assert(!function->boundary_callable);
        assert(function->boundary_diagnostics[0] ==
               "boundary-callable functions must be noexcept in v1");
    }

    {
        const auto table = stable_abi::describe_function<^^variadic_sum>();
        const auto* function = root_function(table);
        assert(function != nullptr);
        assert(function->is_variadic);
        assert(!function->boundary_callable);
        assert(function->boundary_diagnostics[0] ==
               "variadic functions are not boundary-callable in v1");
    }

    {
        const auto table = stable_abi::describe_function<^^invalid_non_trivial_arg>();
        const auto* function = root_function(table);
        assert(function != nullptr);
        assert(!function->boundary_callable);
        assert(function->boundary_diagnostics[0] ==
               "function parameter type is not boundary-transportable in v1");
    }

    {
        const auto left = stable_abi::describe_function_signature<
            double(std::span<const double>) noexcept>();
        const auto right = stable_abi::describe_function_signature<
            double(std::span<const double>) noexcept>();
        assert(stable_abi::to_canonical_json(left) ==
               stable_abi::to_canonical_json(right));
        assert(stable_abi::hash_canonical_json(left) ==
               stable_abi::hash_canonical_json(right));
    }

    return 0;
}
