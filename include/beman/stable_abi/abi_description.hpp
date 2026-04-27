// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_STABLE_ABI_ABI_DESCRIPTION_HPP
#define BEMAN_STABLE_ABI_ABI_DESCRIPTION_HPP

#include <beman/stable_abi/crc32.hpp>

#if !__has_include(<meta>)
#error "beman.stable_abi currently requires a standard library that provides <meta>."
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <meta>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace beman::stable_abi {
namespace meta = std::meta;

enum class type_kind {
    builtin,
    enumeration,
    record,
    span_view,
    array,
    pointer,
    lvalue_reference,
    rvalue_reference,
    unsupported
};

enum class transport_kind {
    unsupported,
    by_value_scalar,
    by_value_record,
    borrowed_view
};

enum class passing_kind {
    unsupported,
    by_value,
    borrowed_view,
    mutable_borrowed_view
};

enum class calling_convention { c_boundary };

enum class exception_policy { noexcept_, may_throw };

enum class description_mode { native, boundary };

struct enum_enumerator_description {
    std::string name;
    std::string value;
};

struct record_field_description {
    std::string name;
    std::string type_id;
    std::size_t offset_bytes = 0;
    std::size_t size_bytes = 0;
    std::size_t align_bytes = 0;
};

struct type_description {
    std::string id;
    std::string display_name;
    type_kind kind = type_kind::unsupported;
    transport_kind boundary_transport_kind = transport_kind::unsupported;
    bool boundary_transportable = false;
    std::string boundary_diagnostic;
    std::size_t size_bytes = 0;
    std::size_t align_bytes = 0;
    bool is_scoped_enum = false;
    std::string underlying_type_id;
    std::vector<enum_enumerator_description> enumerators;
    std::vector<record_field_description> fields;
    std::string element_type_id;
    bool mutable_view = false;
    bool dynamic_extent = true;
    std::size_t extent_value = 0;
    std::size_t array_extent = 0;
    std::string referenced_display_name;
};

struct function_parameter_description {
    std::string name;
    std::string type_id;
    passing_kind passing = passing_kind::unsupported;
};

struct function_result_description {
    std::string type_id;
    passing_kind passing = passing_kind::unsupported;
    bool is_void = false;
};

struct function_description {
    std::string id;
    std::string signature_id;
    std::string display_name;
    std::string qualified_name;
    calling_convention convention = calling_convention::c_boundary;
    exception_policy exception = exception_policy::may_throw;
    bool is_variadic = false;
    std::vector<function_parameter_description> params;
    function_result_description result;
    bool boundary_callable = false;
    std::vector<std::string> boundary_diagnostics;
};

struct abi_spec_table {
    std::uint32_t schema_version = 1;
    description_mode mode = description_mode::boundary;
    std::string root_type_id;
    std::string root_function_id;
    std::vector<type_description> types;
    std::vector<function_description> functions;
};

struct boundary_check_result {
    bool accepted = false;
    std::vector<std::string> diagnostics;
};

namespace detail {
constexpr std::size_t no_index = std::numeric_limits<std::size_t>::max();

template <typename T>
struct span_traits {
    static constexpr bool is_span = false;
};

template <typename Element, std::size_t Extent>
struct span_traits<std::span<Element, Extent>> {
    static constexpr bool is_span = true;
    using element_type = Element;
    static constexpr std::size_t extent = Extent;
    static constexpr bool mutable_view = !std::is_const_v<Element>;
};

template <typename T>
inline constexpr bool is_span_v = span_traits<std::remove_cv_t<T>>::is_span;

template <meta::info Reflection>
consteval auto reflected_type() {
    return meta::type_of(Reflection);
}

template <meta::info Reflection>
consteval auto parameter_type_reflection() {
    if constexpr (meta::is_type(Reflection)) {
        return Reflection;
    } else {
        return meta::type_of(Reflection);
    }
}

template <meta::info Reflection>
consteval auto reflected_return_type() {
    return meta::return_type_of(Reflection);
}

template <meta::info Reflection>
inline constexpr auto reflected_parameters_v =
    std::define_static_array(meta::parameters_of(Reflection));

template <meta::info Reflection>
inline constexpr auto reflected_enumerators_v =
    std::define_static_array(meta::enumerators_of(Reflection));

template <meta::info Reflection>
inline constexpr auto reflected_fields_v = [] {
    constexpr auto context = meta::access_context::unchecked();
    return std::define_static_array(
        meta::nonstatic_data_members_of(Reflection, context));
}();

template <meta::info Reflection>
consteval bool reflection_has_identifier() {
    return meta::has_identifier(Reflection);
}

template <meta::info Reflection>
consteval std::string_view reflection_identifier() {
    return meta::identifier_of(Reflection);
}

template <meta::info Reflection>
consteval std::string_view reflection_display_string() {
    return meta::display_string_of(Reflection);
}

template <meta::info Reflection>
consteval bool reflection_is_noexcept() {
    return meta::is_noexcept(Reflection);
}

template <meta::info Reflection>
consteval bool reflection_has_ellipsis_parameter() {
    return meta::has_ellipsis_parameter(Reflection);
}

inline std::string escape_json(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (const unsigned char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            if (ch < 0x20) {
                std::ostringstream out;
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<unsigned int>(ch);
                escaped += out.str();
            } else {
                escaped.push_back(static_cast<char>(ch));
            }
            break;
        }
    }
    return escaped;
}

inline std::string json_string(std::string_view value) {
    return "\"" + escape_json(value) + "\"";
}

inline std::string indent(std::size_t depth) {
    return std::string(depth * 2, ' ');
}

inline std::string hex_id(std::string_view prefix, std::uint64_t hash) {
    std::ostringstream out;
    out << prefix << std::hex << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}

inline std::uint32_t runtime_crc32(std::string_view value) {
    return detail::crc32(value);
}

constexpr std::uint64_t runtime_fnv1a_64(std::string_view value) {
    std::uint64_t hash = 14695981039346656037ull;
    for (const unsigned char ch : value) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= 1099511628211ull;
    }
    return hash;
}

inline std::string to_string(type_kind kind) {
    switch (kind) {
    case type_kind::builtin:
        return "builtin";
    case type_kind::enumeration:
        return "enum";
    case type_kind::record:
        return "record";
    case type_kind::span_view:
        return "span_view";
    case type_kind::array:
        return "array";
    case type_kind::pointer:
        return "pointer";
    case type_kind::lvalue_reference:
        return "reference";
    case type_kind::rvalue_reference:
        return "reference";
    case type_kind::unsupported:
        return "unsupported";
    }
    return "unsupported";
}

inline std::string to_string(transport_kind kind) {
    switch (kind) {
    case transport_kind::unsupported:
        return "unsupported";
    case transport_kind::by_value_scalar:
        return "by_value_scalar";
    case transport_kind::by_value_record:
        return "by_value_record";
    case transport_kind::borrowed_view:
        return "borrowed_view";
    }
    return "unsupported";
}

inline std::string to_string(passing_kind kind) {
    switch (kind) {
    case passing_kind::unsupported:
        return "unsupported";
    case passing_kind::by_value:
        return "by_value";
    case passing_kind::borrowed_view:
        return "borrowed_view";
    case passing_kind::mutable_borrowed_view:
        return "mutable_borrowed_view";
    }
    return "unsupported";
}

inline std::string to_string(calling_convention convention) {
    switch (convention) {
    case calling_convention::c_boundary:
        return "c_boundary";
    }
    return "c_boundary";
}

inline std::string to_string(exception_policy policy) {
    switch (policy) {
    case exception_policy::noexcept_:
        return "noexcept";
    case exception_policy::may_throw:
        return "may_throw";
    }
    return "may_throw";
}

inline std::string to_string(description_mode mode) {
    switch (mode) {
    case description_mode::native:
        return "native";
    case description_mode::boundary:
        return "boundary";
    }
    return "boundary";
}

template <typename Integer>
std::string integer_to_string(Integer value) {
    std::ostringstream out;
    out << value;
    return out.str();
}

template <meta::info Type>
consteval std::string_view builtin_name() {
    if constexpr (meta::is_same_type(Type, ^^void)) {
        return "void";
    } else if constexpr (meta::is_same_type(Type, ^^bool)) {
        return "bool";
    } else if constexpr (meta::is_same_type(Type, ^^char)) {
        return "char";
    } else if constexpr (meta::is_same_type(Type, ^^signed char)) {
        return "i8";
    } else if constexpr (meta::is_same_type(Type, ^^unsigned char)) {
        return "u8";
    } else if constexpr (meta::is_same_type(Type, ^^short)) {
        return "i16";
    } else if constexpr (meta::is_same_type(Type, ^^unsigned short)) {
        return "u16";
    } else if constexpr (meta::is_same_type(Type, ^^int)) {
        return "i32";
    } else if constexpr (meta::is_same_type(Type, ^^unsigned int)) {
        return "u32";
    } else if constexpr (meta::is_same_type(Type, ^^long)) {
        return meta::size_of(Type) == 8 ? "i64" : "i32";
    } else if constexpr (meta::is_same_type(Type, ^^unsigned long)) {
        return meta::size_of(Type) == 8 ? "u64" : "u32";
    } else if constexpr (meta::is_same_type(Type, ^^long long)) {
        return "i64";
    } else if constexpr (meta::is_same_type(Type, ^^unsigned long long)) {
        return "u64";
    } else if constexpr (meta::is_same_type(Type, ^^wchar_t)) {
        return "wchar";
    } else if constexpr (meta::is_same_type(Type, ^^char8_t)) {
        return "char8";
    } else if constexpr (meta::is_same_type(Type, ^^char16_t)) {
        return "char16";
    } else if constexpr (meta::is_same_type(Type, ^^char32_t)) {
        return "char32";
    } else if constexpr (meta::is_same_type(Type, ^^float)) {
        return "f32";
    } else if constexpr (meta::is_same_type(Type, ^^double)) {
        return "f64";
    } else if constexpr (meta::is_same_type(Type, ^^long double)) {
        return "f128";
    } else if constexpr (meta::is_same_type(Type, ^^std::nullptr_t)) {
        return "nullptr";
    } else {
        return meta::display_string_of(Type);
    }
}

template <meta::info Type>
consteval bool is_builtin_type() {
    return meta::is_void_type(Type) || meta::is_fundamental_type(Type);
}

template <meta::info Type>
std::string display_name_for_type() {
    if constexpr (is_builtin_type<Type>()) {
        return std::string(builtin_name<Type>());
    } else if constexpr (meta::has_identifier(Type)) {
        return std::string(meta::identifier_of(Type));
    } else {
        return std::string(meta::display_string_of(Type));
    }
}

struct internal_record_field {
    std::string name;
    std::size_t type_index = no_index;
    std::size_t offset_bytes = 0;
    std::size_t size_bytes = 0;
    std::size_t align_bytes = 0;
};

struct internal_type_description {
    std::string display_name;
    type_kind kind = type_kind::unsupported;
    transport_kind boundary_transport_kind = transport_kind::unsupported;
    bool boundary_transportable = false;
    std::string boundary_diagnostic;
    std::size_t size_bytes = 0;
    std::size_t align_bytes = 0;
    bool is_scoped_enum = false;
    std::size_t underlying_type_index = no_index;
    std::vector<enum_enumerator_description> enumerators;
    std::vector<internal_record_field> fields;
    std::size_t element_type_index = no_index;
    bool mutable_view = false;
    bool dynamic_extent = true;
    std::size_t extent_value = 0;
    std::size_t array_extent = 0;
    std::string referenced_display_name;
};

struct internal_function_parameter {
    std::string name;
    std::size_t type_index = no_index;
    passing_kind passing = passing_kind::unsupported;
};

struct internal_function_result {
    std::size_t type_index = no_index;
    passing_kind passing = passing_kind::unsupported;
    bool is_void = false;
};

struct internal_function_description {
    std::string display_name;
    std::string qualified_name;
    calling_convention convention = calling_convention::c_boundary;
    exception_policy exception = exception_policy::may_throw;
    bool is_variadic = false;
    std::vector<internal_function_parameter> params;
    internal_function_result result;
    bool boundary_callable = false;
    std::vector<std::string> boundary_diagnostics;
};

class abi_builder {
public:
    explicit abi_builder(description_mode mode) : mode_(mode) {}

    template <meta::info Type>
    abi_spec_table describe_root_type() {
        const std::size_t root_type = ensure_type<Type>();
        abi_spec_table table = finalize();
        table.root_type_id = materialized_type_ids_[root_type];
        return table;
    }

    template <typename FunctionType>
    abi_spec_table describe_root_function_signature() {
        constexpr auto reflected_type = function_signature_reflection<FunctionType>();
        const std::size_t function_index = append_function_from_signature<reflected_type>();
        abi_spec_table table = finalize();
        table.root_function_id = materialized_function_ids_[function_index];
        return table;
    }

    template <meta::info Reflection>
    abi_spec_table describe_root_function() {
        static_assert(meta::is_function(Reflection),
                      "describe_function requires a function reflection token");
        const std::size_t function_index = append_function_from_entity<Reflection>();
        abi_spec_table table = finalize();
        table.root_function_id = materialized_function_ids_[function_index];
        return table;
    }

private:
    bool is_boundary_mode() const { return mode_ == description_mode::boundary; }
    bool is_native_mode() const { return mode_ == description_mode::native; }

    template <typename T>
    std::size_t ensure_type_from_cpp() {
        return ensure_type<^^T>();
    }

    template <typename T>
    static passing_kind classify_parameter_passing_from_cpp() {
        return classify_parameter_passing<^^T>();
    }

    template <typename T>
    internal_function_result make_result_description_from_cpp() {
        return make_result_description<^^T>();
    }

    template <typename T>
    void validate_boundary_parameter_from_cpp(
        internal_function_description& desc) {
        validate_boundary_parameter<^^T>(desc);
    }

    template <typename T>
    void validate_boundary_result_from_cpp(
        internal_function_description& desc) {
        validate_boundary_result<^^T>(desc);
    }

    template <typename T>
    bool is_record_field_boundary_transportable_from_cpp() {
        return is_record_field_boundary_transportable<^^T>();
    }

    template <typename FunctionType>
    static consteval meta::info function_signature_reflection() {
        if constexpr (meta::is_function_type(^^FunctionType)) {
            return ^^FunctionType;
        } else if constexpr (meta::is_pointer_type(^^FunctionType) &&
                             meta::is_function_type(
                                 meta::remove_pointer(^^FunctionType))) {
            return meta::remove_pointer(^^FunctionType);
        } else {
            static_assert(std::is_function_v<FunctionType>,
                          "describe_function_signature requires a function type");
            return ^^FunctionType;
        }
    }

    template <meta::info Type>
    std::size_t ensure_type() {
        const std::string key = std::string(meta::display_string_of(Type));
        if (const auto found = type_index_by_key_.find(key);
            found != type_index_by_key_.end()) {
            return found->second;
        }

        const std::size_t index = internal_types_.size();
        type_index_by_key_.emplace(key, index);
        internal_types_.push_back({});

        internal_types_[index].display_name = display_name_for_type<Type>();
        if constexpr (meta::is_void_type(Type) || meta::is_reference_type(Type)) {
            internal_types_[index].size_bytes = 0;
            internal_types_[index].align_bytes = 0;
        } else {
            internal_types_[index].size_bytes = meta::size_of(Type);
            internal_types_[index].align_bytes = meta::alignment_of(Type);
        }

        if constexpr (is_builtin_type<Type>()) {
            internal_types_[index].kind = type_kind::builtin;
            if (is_boundary_mode()) {
                internal_types_[index].boundary_transportable =
                    !meta::is_void_type(Type);
                internal_types_[index].boundary_transport_kind =
                    meta::is_void_type(Type) ? transport_kind::unsupported
                                             : transport_kind::by_value_scalar;
                if constexpr (meta::is_void_type(Type)) {
                    internal_types_[index].boundary_diagnostic =
                        "void is only valid as a function result in v1";
                }
            }
        } else if constexpr (meta::is_enum_type(Type)) {
            using enum_type = typename [:Type:];
            using underlying_type = std::underlying_type_t<enum_type>;

            internal_types_[index].kind = type_kind::enumeration;
            if (is_boundary_mode()) {
                internal_types_[index].boundary_transportable = true;
                internal_types_[index].boundary_transport_kind =
                    transport_kind::by_value_scalar;
            }
            internal_types_[index].is_scoped_enum =
                meta::is_scoped_enum_type(Type);
            internal_types_[index].underlying_type_index =
                ensure_type<^^underlying_type>();

            template for (constexpr auto enumerator : reflected_enumerators_v<Type>) {
                enum_enumerator_description enumerator_desc;
                if constexpr (reflection_has_identifier<enumerator>()) {
                    enumerator_desc.name =
                        std::string(reflection_identifier<enumerator>());
                } else {
                    enumerator_desc.name =
                        std::string(reflection_display_string<enumerator>());
                }
                const enum_type enum_value =
                    meta::extract<enum_type>(meta::constant_of(enumerator));
                const underlying_type underlying_value =
                    static_cast<underlying_type>(enum_value);
                enumerator_desc.value = integer_to_string(underlying_value);
                internal_types_[index].enumerators.push_back(
                    std::move(enumerator_desc));
            }
        } else if constexpr (span_traits<typename [:Type:]>::is_span && false) {
            // Unused: handled below so native mode can preserve the native layout.
        } else if constexpr (span_traits<typename [:Type:]>::is_span && true) {
            using span_type = typename [:Type:];
            using element_type =
                typename span_traits<std::remove_cv_t<span_type>>::element_type;
            if (is_boundary_mode()) {
                internal_types_[index].kind = type_kind::span_view;
                internal_types_[index].boundary_transportable = true;
                internal_types_[index].boundary_transport_kind =
                    transport_kind::borrowed_view;
                internal_types_[index].element_type_index =
                    ensure_type_from_cpp<std::remove_cv_t<element_type>>();
                internal_types_[index].mutable_view =
                    span_traits<std::remove_cv_t<span_type>>::mutable_view;
                internal_types_[index].dynamic_extent =
                    span_traits<std::remove_cv_t<span_type>>::extent ==
                    std::dynamic_extent;
                internal_types_[index].extent_value =
                    internal_types_[index].dynamic_extent
                        ? 0
                        : span_traits<std::remove_cv_t<span_type>>::extent;
            } else {
                internal_types_[index].kind = type_kind::record;
                bool recursively_transportable = true;
                template for (constexpr auto field : reflected_fields_v<Type>) {
                    using field_type = typename [:reflected_type<field>() :];
                    internal_record_field field_desc;
                    if constexpr (reflection_has_identifier<field>()) {
                        field_desc.name =
                            std::string(reflection_identifier<field>());
                    } else {
                        field_desc.name =
                            std::string(reflection_display_string<field>());
                    }
                    field_desc.type_index = ensure_type_from_cpp<field_type>();
                    const auto offset = meta::offset_of(field);
                    field_desc.offset_bytes =
                        static_cast<std::size_t>(offset.bytes);
                    field_desc.size_bytes = meta::size_of(^^field_type);
                    field_desc.align_bytes = meta::alignment_of(^^field_type);
                    internal_types_[index].fields.push_back(std::move(field_desc));
                    recursively_transportable =
                        recursively_transportable &&
                        is_record_field_boundary_transportable_from_cpp<
                            field_type>();
                }
                (void)recursively_transportable;
            }
        } else if constexpr (meta::is_bounded_array_type(Type)) {
            using element_type = typename [:meta::remove_extent(Type):];
            internal_types_[index].kind = type_kind::array;
            if (is_boundary_mode()) {
                internal_types_[index].boundary_transportable = false;
                internal_types_[index].boundary_transport_kind =
                    transport_kind::unsupported;
                internal_types_[index].boundary_diagnostic =
                    "fixed-size arrays are only boundary-callable inside accepted "
                    "trivial records in v1";
            }
            internal_types_[index].element_type_index =
                ensure_type_from_cpp<element_type>();
            internal_types_[index].array_extent = meta::extent(Type);
        } else if constexpr (meta::is_pointer_type(Type)) {
            internal_types_[index].kind = type_kind::pointer;
            if (is_boundary_mode()) {
                internal_types_[index].boundary_transportable = false;
                internal_types_[index].boundary_transport_kind =
                    transport_kind::unsupported;
                internal_types_[index].boundary_diagnostic =
                    "pointer types are not boundary-transportable in v1";
            }
            internal_types_[index].referenced_display_name =
                std::string(meta::display_string_of(meta::remove_pointer(Type)));
        } else if constexpr (meta::is_lvalue_reference_type(Type)) {
            internal_types_[index].kind = type_kind::lvalue_reference;
            if (is_boundary_mode()) {
                internal_types_[index].boundary_transportable = false;
                internal_types_[index].boundary_transport_kind =
                    transport_kind::unsupported;
                internal_types_[index].boundary_diagnostic =
                    "reference types are not boundary-transportable in v1";
            }
            internal_types_[index].referenced_display_name = std::string(
                meta::display_string_of(meta::remove_reference(Type)));
        } else if constexpr (meta::is_rvalue_reference_type(Type)) {
            internal_types_[index].kind = type_kind::rvalue_reference;
            if (is_boundary_mode()) {
                internal_types_[index].boundary_transportable = false;
                internal_types_[index].boundary_transport_kind =
                    transport_kind::unsupported;
                internal_types_[index].boundary_diagnostic =
                    "reference types are not boundary-transportable in v1";
            }
            internal_types_[index].referenced_display_name = std::string(
                meta::display_string_of(meta::remove_reference(Type)));
        } else if constexpr (meta::is_class_type(Type)) {
            internal_types_[index].kind = type_kind::record;

            bool recursively_transportable = true;
            template for (constexpr auto field : reflected_fields_v<Type>) {
                using field_type = typename [:reflected_type<field>() :];
                internal_record_field field_desc;
                if constexpr (reflection_has_identifier<field>()) {
                    field_desc.name = std::string(reflection_identifier<field>());
                } else {
                    field_desc.name =
                        std::string(reflection_display_string<field>());
                }
                field_desc.type_index = ensure_type_from_cpp<field_type>();
                const auto offset = meta::offset_of(field);
                field_desc.offset_bytes = static_cast<std::size_t>(offset.bytes);
                if constexpr (meta::is_reference_type(^^field_type)) {
                    field_desc.size_bytes = 0;
                    field_desc.align_bytes = 0;
                } else {
                    field_desc.size_bytes = meta::size_of(^^field_type);
                    field_desc.align_bytes = meta::alignment_of(^^field_type);
                }
                internal_types_[index].fields.push_back(std::move(field_desc));
                recursively_transportable =
                    recursively_transportable &&
                    is_record_field_boundary_transportable_from_cpp<field_type>();
            }

            const bool trivial_record =
                meta::is_trivial_type(Type) &&
                meta::is_standard_layout_type(Type) &&
                meta::is_trivially_copyable_type(Type) &&
                meta::is_trivially_destructible_type(Type);

            if (is_boundary_mode()) {
                internal_types_[index].boundary_transportable =
                    trivial_record && recursively_transportable;
                internal_types_[index].boundary_transport_kind =
                    internal_types_[index].boundary_transportable
                        ? transport_kind::by_value_record
                        : transport_kind::unsupported;

                if (!trivial_record) {
                    internal_types_[index].boundary_diagnostic =
                        "only trivial standard-layout records are "
                        "boundary-transportable by value in v1";
                } else if (!recursively_transportable) {
                    internal_types_[index].boundary_diagnostic =
                        "record contains a field that is not boundary-transportable "
                        "in v1";
                }
            }
        } else {
            internal_types_[index].kind = type_kind::unsupported;
            if (is_boundary_mode()) {
                internal_types_[index].boundary_transportable = false;
                internal_types_[index].boundary_transport_kind =
                    transport_kind::unsupported;
                internal_types_[index].boundary_diagnostic =
                    "type is describable but not boundary-transportable in v1";
            }
        }

        return index;
    }

    template <meta::info Type>
    bool is_record_field_boundary_transportable() {
        if constexpr (meta::is_void_type(Type)) {
            return false;
        } else if constexpr (meta::is_bounded_array_type(Type)) {
            return is_record_field_boundary_transportable<meta::remove_extent(
                Type)>();
        } else if constexpr (is_builtin_type<Type>()) {
            return !meta::is_void_type(Type);
        } else if constexpr (meta::is_enum_type(Type)) {
            return true;
        } else if constexpr (span_traits<typename [:Type:]>::is_span) {
            return false;
        } else if constexpr (meta::is_pointer_type(Type) ||
                             meta::is_reference_type(Type)) {
            return false;
        } else if constexpr (meta::is_class_type(Type)) {
            bool recursively_transportable = meta::is_trivial_type(Type) &&
                                             meta::is_standard_layout_type(Type) &&
                                             meta::is_trivially_copyable_type(
                                                 Type) &&
                                             meta::is_trivially_destructible_type(
                                                 Type);
            template for (constexpr auto field : reflected_fields_v<Type>) {
                using field_type = typename [:reflected_type<field>() :];
                recursively_transportable =
                    recursively_transportable &&
                    is_record_field_boundary_transportable_from_cpp<field_type>();
            }
            return recursively_transportable;
        } else {
            return false;
        }
    }

    template <meta::info Signature>
    std::size_t append_function_from_signature() {
        internal_function_description desc;
        desc.convention = calling_convention::c_boundary;
        desc.exception = reflection_is_noexcept<Signature>()
                             ? exception_policy::noexcept_
                             : exception_policy::may_throw;
        desc.is_variadic = reflection_has_ellipsis_parameter<Signature>();
        desc.boundary_callable = true;

        std::size_t parameter_index = 0;
        template for (constexpr auto parameter : reflected_parameters_v<Signature>) {
            desc.params.push_back(
                make_parameter_description<parameter>("arg", parameter_index));
            ++parameter_index;
        }

        using result_type = typename [:reflected_return_type<Signature>() :];
        desc.result = make_result_description_from_cpp<result_type>();
        validate_boundary_function<Signature>(desc);
        internal_functions_.push_back(std::move(desc));
        return internal_functions_.size() - 1;
    }

    template <meta::info Reflection>
    std::size_t append_function_from_entity() {
        static_assert(!meta::is_class_member(Reflection),
                      "describe_function v1 supports only free functions");
        internal_function_description desc;
        if constexpr (reflection_has_identifier<Reflection>()) {
            desc.display_name = std::string(reflection_identifier<Reflection>());
        } else {
            desc.display_name =
                std::string(reflection_display_string<Reflection>());
        }
        desc.qualified_name = std::string(reflection_display_string<Reflection>());
        desc.convention = calling_convention::c_boundary;
        desc.exception = reflection_is_noexcept<Reflection>()
                             ? exception_policy::noexcept_
                             : exception_policy::may_throw;
        desc.is_variadic = reflection_has_ellipsis_parameter<Reflection>();
        desc.boundary_callable = true;

        std::size_t parameter_index = 0;
        template for (constexpr auto parameter : reflected_parameters_v<Reflection>) {
            desc.params.push_back(
                make_parameter_description<parameter>("arg", parameter_index));
            ++parameter_index;
        }

        using result_type = typename [:reflected_return_type<Reflection>() :];
        desc.result = make_result_description_from_cpp<result_type>();
        validate_boundary_function<Reflection>(desc);
        internal_functions_.push_back(std::move(desc));
        return internal_functions_.size() - 1;
    }

    template <meta::info Parameter>
    internal_function_parameter make_parameter_description(
        std::string base_name, std::size_t position) {
        using parameter_type =
            typename [:parameter_type_reflection<Parameter>() :];
        internal_function_parameter parameter_desc;
        if constexpr (reflection_has_identifier<Parameter>()) {
            parameter_desc.name = std::string(reflection_identifier<Parameter>());
        } else {
            parameter_desc.name = base_name + std::to_string(position);
        }
        parameter_desc.type_index = ensure_type_from_cpp<parameter_type>();
        parameter_desc.passing =
            classify_parameter_passing_from_cpp<parameter_type>();
        return parameter_desc;
    }

    template <meta::info Type>
    internal_function_result make_result_description() {
        internal_function_result result_desc;
        result_desc.type_index = ensure_type<Type>();
        result_desc.is_void = meta::is_void_type(Type);
        result_desc.passing = classify_result_passing<Type>();
        return result_desc;
    }

    template <meta::info Type>
    static passing_kind classify_parameter_passing() {
        if constexpr (span_traits<typename [:Type:]>::is_span) {
            return span_traits<typename [:Type:]>::mutable_view
                       ? passing_kind::mutable_borrowed_view
                       : passing_kind::borrowed_view;
        } else if constexpr (is_builtin_type<Type>() || meta::is_enum_type(Type) ||
                             meta::is_class_type(Type)) {
            return passing_kind::by_value;
        } else {
            return passing_kind::unsupported;
        }
    }

    template <meta::info Type>
    static passing_kind classify_result_passing() {
        if constexpr (meta::is_void_type(Type)) {
            return passing_kind::by_value;
        } else if constexpr (span_traits<typename [:Type:]>::is_span) {
            return span_traits<typename [:Type:]>::mutable_view
                       ? passing_kind::mutable_borrowed_view
                       : passing_kind::borrowed_view;
        } else if constexpr (is_builtin_type<Type>() || meta::is_enum_type(Type) ||
                             meta::is_class_type(Type)) {
            return passing_kind::by_value;
        } else {
            return passing_kind::unsupported;
        }
    }

    template <meta::info Signature>
    void validate_boundary_function(internal_function_description& desc) {
        if (!reflection_is_noexcept<Signature>()) {
            desc.boundary_callable = false;
            desc.boundary_diagnostics.push_back(
                "boundary-callable functions must be noexcept in v1");
        }
        if (reflection_has_ellipsis_parameter<Signature>()) {
            desc.boundary_callable = false;
            desc.boundary_diagnostics.push_back(
                "variadic functions are not boundary-callable in v1");
        }

        template for (constexpr auto parameter : reflected_parameters_v<Signature>) {
            using parameter_type =
                typename [:parameter_type_reflection<parameter>() :];
            validate_boundary_parameter_from_cpp<parameter_type>(desc);
        }
        using result_type = typename [:reflected_return_type<Signature>() :];
        validate_boundary_result_from_cpp<result_type>(desc);
    }

    template <meta::info Type>
    void validate_boundary_parameter(internal_function_description& desc) {
        const std::size_t type_index = ensure_type<Type>();
        if constexpr (span_traits<typename [:Type:]>::is_span) {
            using span_type = typename [:Type:];
            using element_type =
                typename span_traits<std::remove_cv_t<span_type>>::element_type;
            if (!is_record_field_boundary_transportable<
                    ^^std::remove_cv_t<element_type>>()) {
                desc.boundary_callable = false;
                desc.boundary_diagnostics.push_back(
                    "span<T> parameters require a boundary-transportable "
                    "element type in v1");
            }
        } else if constexpr (meta::is_bounded_array_type(Type)) {
            desc.boundary_callable = false;
            desc.boundary_diagnostics.push_back(
                "array parameters are not boundary-callable in v1");
        } else if (!internal_types_[type_index].boundary_transportable) {
            desc.boundary_callable = false;
            desc.boundary_diagnostics.push_back(
                "function parameter type is not boundary-transportable in v1");
        }
    }

    template <meta::info Type>
    void validate_boundary_result(internal_function_description& desc) {
        const std::size_t type_index = ensure_type<Type>();
        if constexpr (meta::is_void_type(Type)) {
            return;
        } else if constexpr (span_traits<typename [:Type:]>::is_span) {
            desc.boundary_callable = false;
            desc.boundary_diagnostics.push_back(
                "span<T> results are not boundary-callable in v1");
        } else if constexpr (meta::is_bounded_array_type(Type)) {
            desc.boundary_callable = false;
            desc.boundary_diagnostics.push_back(
                "array results are not boundary-callable in v1");
        } else if (!internal_types_[type_index].boundary_transportable) {
            desc.boundary_callable = false;
            desc.boundary_diagnostics.push_back(
                "function result type is not boundary-transportable in v1");
        }
    }

    std::string canonical_type_identity_json(std::size_t index,
                                             std::vector<std::string>& cache,
                                             std::vector<bool>& in_progress) {
        if (!cache[index].empty()) {
            return cache[index];
        }
        assert(!in_progress[index] &&
               "unexpected recursive identity cycle in type description");
        in_progress[index] = true;

        const internal_type_description& desc = internal_types_[index];
        std::ostringstream out;
        out << "{";
        out << "\"kind\":" << json_string(to_string(desc.kind)) << ",";
        out << "\"transport_kind\":"
            << json_string(to_string(desc.boundary_transport_kind)) << ",";
        out << "\"boundary_transportable\":"
            << (desc.boundary_transportable ? "true" : "false");

        switch (desc.kind) {
        case type_kind::builtin:
            out << ",\"builtin_name\":" << json_string(desc.display_name);
            break;
        case type_kind::enumeration:
            out << ",\"underlying_type_id\":"
                << json_string(compute_type_id(desc.underlying_type_index, cache,
                                               in_progress));
            out << ",\"is_scoped\":"
                << (desc.is_scoped_enum ? "true" : "false");
            out << ",\"enumerators\":[";
            for (std::size_t i = 0; i != desc.enumerators.size(); ++i) {
                if (i != 0) {
                    out << ",";
                }
                out << "{"
                    << "\"name\":" << json_string(desc.enumerators[i].name)
                    << ",\"value\":" << desc.enumerators[i].value << "}";
            }
            out << "]";
            break;
        case type_kind::record:
            out << ",\"size_bytes\":" << desc.size_bytes;
            out << ",\"align_bytes\":" << desc.align_bytes;
            out << ",\"fields\":[";
            for (std::size_t i = 0; i != desc.fields.size(); ++i) {
                if (i != 0) {
                    out << ",";
                }
                out << "{"
                    << "\"name\":" << json_string(desc.fields[i].name)
                    << ",\"type_id\":"
                    << json_string(compute_type_id(desc.fields[i].type_index, cache,
                                                   in_progress))
                    << ",\"offset_bytes\":" << desc.fields[i].offset_bytes
                    << ",\"size_bytes\":" << desc.fields[i].size_bytes
                    << ",\"align_bytes\":" << desc.fields[i].align_bytes << "}";
            }
            out << "]";
            break;
        case type_kind::span_view:
            out << ",\"element_type_id\":"
                << json_string(compute_type_id(desc.element_type_index, cache,
                                               in_progress));
            out << ",\"mutable\":"
                << (desc.mutable_view ? "true" : "false");
            out << ",\"dynamic_extent\":"
                << (desc.dynamic_extent ? "true" : "false");
            if (!desc.dynamic_extent) {
                out << ",\"extent\":" << desc.extent_value;
            }
            break;
        case type_kind::array:
            out << ",\"element_type_id\":"
                << json_string(compute_type_id(desc.element_type_index, cache,
                                               in_progress));
            out << ",\"extent\":" << desc.array_extent;
            break;
        case type_kind::pointer:
        case type_kind::lvalue_reference:
        case type_kind::rvalue_reference:
        case type_kind::unsupported:
            if (!desc.referenced_display_name.empty()) {
                out << ",\"referenced_display_name\":"
                    << json_string(desc.referenced_display_name);
            }
            break;
        }

        out << "}";
        in_progress[index] = false;
        cache[index] = out.str();
        return cache[index];
    }

    std::string compute_type_id(std::size_t index, std::vector<std::string>& cache,
                                std::vector<bool>& in_progress) {
        if (!materialized_type_ids_[index].empty()) {
            return materialized_type_ids_[index];
        }
        const std::string canonical = canonical_type_identity_json(
            index, cache, in_progress);
        materialized_type_ids_[index] =
            hex_id("type:", runtime_fnv1a_64(canonical));
        return materialized_type_ids_[index];
    }

    std::string canonical_function_signature_json(
        const internal_function_description& desc) {
        std::ostringstream out;
        out << "{";
        out << "\"calling_convention\":"
            << json_string(to_string(desc.convention)) << ",";
        out << "\"exception_policy\":"
            << json_string(to_string(desc.exception)) << ",";
        out << "\"is_variadic\":" << (desc.is_variadic ? "true" : "false");
        out << ",\"params\":[";
        for (std::size_t i = 0; i != desc.params.size(); ++i) {
            if (i != 0) {
                out << ",";
            }
            out << "{"
                << "\"type_id\":" << json_string(materialized_type_ids_[desc.params[i].type_index])
                << ",\"passing\":" << json_string(to_string(desc.params[i].passing))
                << "}";
        }
        out << "],\"result\":{";
        out << "\"type_id\":" << json_string(materialized_type_ids_[desc.result.type_index])
            << ",\"passing\":" << json_string(to_string(desc.result.passing))
            << ",\"is_void\":" << (desc.result.is_void ? "true" : "false")
            << "}}";
        return out.str();
    }

    std::string canonical_function_identity_json(
        const internal_function_description& desc) {
        std::ostringstream out;
        out << "{";
        out << "\"signature_id\":"
            << json_string(hex_id("fnsig:",
                                  runtime_fnv1a_64(
                                      canonical_function_signature_json(desc))));
        if (!desc.qualified_name.empty()) {
            out << ",\"qualified_name\":" << json_string(desc.qualified_name);
        } else if (!desc.display_name.empty()) {
            out << ",\"display_name\":" << json_string(desc.display_name);
        }
        out << "}";
        return out.str();
    }

    abi_spec_table finalize() {
        materialized_type_ids_.assign(internal_types_.size(), {});
        materialized_function_ids_.assign(internal_functions_.size(), {});

        std::vector<std::string> canonical_cache(internal_types_.size());
        std::vector<bool> in_progress(internal_types_.size(), false);

        for (std::size_t index = 0; index != internal_types_.size(); ++index) {
            compute_type_id(index, canonical_cache, in_progress);
        }

        std::vector<std::string> materialized_function_signature_ids(
            internal_functions_.size());
        for (std::size_t index = 0; index != internal_functions_.size(); ++index) {
            const std::string signature_json =
                canonical_function_signature_json(internal_functions_[index]);
            materialized_function_signature_ids[index] =
                hex_id("fnsig:", runtime_fnv1a_64(signature_json));
            const std::string identity_json =
                internal_functions_[index].qualified_name.empty() &&
                        internal_functions_[index].display_name.empty()
                    ? signature_json
                    : canonical_function_identity_json(internal_functions_[index]);
            materialized_function_ids_[index] =
                hex_id("fn:", runtime_fnv1a_64(identity_json));
        }

        abi_spec_table table;
        table.mode = mode_;
        table.types.reserve(internal_types_.size());
        table.functions.reserve(internal_functions_.size());

        for (std::size_t index = 0; index != internal_types_.size(); ++index) {
            const internal_type_description& source = internal_types_[index];
            type_description target;
            target.id = materialized_type_ids_[index];
            target.display_name = source.display_name;
            target.kind = source.kind;
            target.boundary_transport_kind = source.boundary_transport_kind;
            target.boundary_transportable = source.boundary_transportable;
            target.boundary_diagnostic = source.boundary_diagnostic;
            target.size_bytes = source.size_bytes;
            target.align_bytes = source.align_bytes;
            target.is_scoped_enum = source.is_scoped_enum;
            target.enumerators = source.enumerators;
            target.mutable_view = source.mutable_view;
            target.dynamic_extent = source.dynamic_extent;
            target.extent_value = source.extent_value;
            target.array_extent = source.array_extent;
            target.referenced_display_name = source.referenced_display_name;

            if (source.underlying_type_index != no_index) {
                target.underlying_type_id =
                    materialized_type_ids_[source.underlying_type_index];
            }
            if (source.element_type_index != no_index) {
                target.element_type_id =
                    materialized_type_ids_[source.element_type_index];
            }

            target.fields.reserve(source.fields.size());
            for (const internal_record_field& field : source.fields) {
                target.fields.push_back(record_field_description{
                    .name = field.name,
                    .type_id = materialized_type_ids_[field.type_index],
                    .offset_bytes = field.offset_bytes,
                    .size_bytes = field.size_bytes,
                    .align_bytes = field.align_bytes,
                });
            }
            table.types.push_back(std::move(target));
        }

        for (std::size_t index = 0; index != internal_functions_.size(); ++index) {
            const internal_function_description& source = internal_functions_[index];
            function_description target;
            target.id = materialized_function_ids_[index];
            target.signature_id = materialized_function_signature_ids[index];
            target.display_name = source.display_name;
            target.qualified_name = source.qualified_name;
            target.convention = source.convention;
            target.exception = source.exception;
            target.is_variadic = source.is_variadic;
            target.boundary_callable = source.boundary_callable;
            target.boundary_diagnostics = source.boundary_diagnostics;

            target.params.reserve(source.params.size());
            for (const internal_function_parameter& param : source.params) {
                target.params.push_back(function_parameter_description{
                    .name = param.name,
                    .type_id = materialized_type_ids_[param.type_index],
                    .passing = param.passing,
                });
            }

            target.result = function_result_description{
                .type_id = materialized_type_ids_[source.result.type_index],
                .passing = source.result.passing,
                .is_void = source.result.is_void,
            };
            table.functions.push_back(std::move(target));
        }

        return table;
    }

    std::unordered_map<std::string, std::size_t> type_index_by_key_;
    std::vector<internal_type_description> internal_types_;
    std::vector<internal_function_description> internal_functions_;
    std::vector<std::string> materialized_type_ids_;
    std::vector<std::string> materialized_function_ids_;
    description_mode mode_;
};

inline std::string join_json_array(const std::vector<std::string>& items,
                                   bool pretty, std::size_t depth) {
    if (items.empty()) {
        return "[]";
    }
    std::ostringstream out;
    out << "[";
    if (pretty) {
        out << "\n";
    }
    for (std::size_t i = 0; i != items.size(); ++i) {
        if (pretty) {
            out << indent(depth + 1);
        }
        out << items[i];
        if (i + 1 != items.size()) {
            out << ",";
        }
        if (pretty) {
            out << "\n";
        }
    }
    if (pretty) {
        out << indent(depth);
    }
    out << "]";
    return out.str();
}

inline std::string serialize_type(const type_description& desc, bool pretty,
                                  std::size_t depth) {
    std::vector<std::string> parts;
    parts.push_back("\"id\":" + json_string(desc.id));
    parts.push_back("\"display_name\":" + json_string(desc.display_name));
    parts.push_back("\"kind\":" + json_string(to_string(desc.kind)));
    parts.push_back("\"transport_kind\":" +
                    json_string(to_string(desc.boundary_transport_kind)));
    parts.push_back("\"boundary_transportable\":" +
                    std::string(desc.boundary_transportable ? "true" : "false"));

    if (!desc.boundary_diagnostic.empty()) {
        parts.push_back("\"boundary_diagnostic\":" +
                        json_string(desc.boundary_diagnostic));
    }

    switch (desc.kind) {
    case type_kind::builtin:
        break;
    case type_kind::enumeration: {
        parts.push_back("\"underlying_type_id\":" +
                        json_string(desc.underlying_type_id));
        parts.push_back("\"is_scoped\":" +
                        std::string(desc.is_scoped_enum ? "true" : "false"));
        std::vector<std::string> enumerators;
        enumerators.reserve(desc.enumerators.size());
        for (const enum_enumerator_description& enumerator : desc.enumerators) {
            enumerators.push_back("{\"name\":" + json_string(enumerator.name) +
                                  ",\"value\":" + enumerator.value + "}");
        }
        parts.push_back("\"enumerators\":" +
                        join_json_array(enumerators, pretty, depth + 1));
        break;
    }
    case type_kind::record: {
        parts.push_back("\"size_bytes\":" + std::to_string(desc.size_bytes));
        parts.push_back("\"align_bytes\":" + std::to_string(desc.align_bytes));
        std::vector<std::string> fields;
        fields.reserve(desc.fields.size());
        for (const record_field_description& field : desc.fields) {
            fields.push_back("{\"name\":" + json_string(field.name) +
                             ",\"type_id\":" + json_string(field.type_id) +
                             ",\"offset_bytes\":" +
                             std::to_string(field.offset_bytes) +
                             ",\"size_bytes\":" +
                             std::to_string(field.size_bytes) +
                             ",\"align_bytes\":" +
                             std::to_string(field.align_bytes) + "}");
        }
        parts.push_back("\"fields\":" +
                        join_json_array(fields, pretty, depth + 1));
        break;
    }
    case type_kind::span_view:
        parts.push_back("\"element_type_id\":" +
                        json_string(desc.element_type_id));
        parts.push_back("\"mutable\":" +
                        std::string(desc.mutable_view ? "true" : "false"));
        parts.push_back("\"dynamic_extent\":" +
                        std::string(desc.dynamic_extent ? "true" : "false"));
        if (!desc.dynamic_extent) {
            parts.push_back("\"extent\":" + std::to_string(desc.extent_value));
        }
        break;
    case type_kind::array:
        parts.push_back("\"element_type_id\":" +
                        json_string(desc.element_type_id));
        parts.push_back("\"extent\":" + std::to_string(desc.array_extent));
        break;
    case type_kind::pointer:
    case type_kind::lvalue_reference:
    case type_kind::rvalue_reference:
    case type_kind::unsupported:
        if (!desc.referenced_display_name.empty()) {
            parts.push_back("\"referenced_display_name\":" +
                            json_string(desc.referenced_display_name));
        }
        break;
    }

    std::ostringstream out;
    out << "{";
    if (pretty) {
        out << "\n";
    }
    for (std::size_t i = 0; i != parts.size(); ++i) {
        if (pretty) {
            out << indent(depth + 1);
        }
        out << parts[i];
        if (i + 1 != parts.size()) {
            out << ",";
        }
        if (pretty) {
            out << "\n";
        }
    }
    if (pretty) {
        out << indent(depth);
    }
    out << "}";
    return out.str();
}

inline std::string serialize_function(const function_description& desc, bool pretty,
                                      std::size_t depth) {
    std::vector<std::string> parts;
    parts.push_back("\"id\":" + json_string(desc.id));
    if (!desc.signature_id.empty()) {
        parts.push_back("\"signature_id\":" + json_string(desc.signature_id));
    }
    parts.push_back("\"display_name\":" + json_string(desc.display_name));
    if (!desc.qualified_name.empty()) {
        parts.push_back("\"qualified_name\":" + json_string(desc.qualified_name));
    }
    parts.push_back("\"calling_convention\":" +
                    json_string(to_string(desc.convention)));
    parts.push_back("\"exception_policy\":" +
                    json_string(to_string(desc.exception)));
    parts.push_back("\"is_variadic\":" +
                    std::string(desc.is_variadic ? "true" : "false"));

    std::vector<std::string> params;
    params.reserve(desc.params.size());
    for (const function_parameter_description& param : desc.params) {
        params.push_back("{\"name\":" + json_string(param.name) +
                         ",\"type_id\":" + json_string(param.type_id) +
                         ",\"passing\":" +
                         json_string(to_string(param.passing)) + "}");
    }
    parts.push_back("\"params\":" + join_json_array(params, pretty, depth + 1));

    parts.push_back(
        "\"result\":{" +
        std::string("\"type_id\":") + json_string(desc.result.type_id) +
        ",\"passing\":" + json_string(to_string(desc.result.passing)) +
        ",\"is_void\":" + (desc.result.is_void ? "true" : "false") + "}");

    parts.push_back("\"boundary_callable\":" +
                    std::string(desc.boundary_callable ? "true" : "false"));
    if (!desc.boundary_diagnostics.empty()) {
        std::vector<std::string> diagnostics;
        diagnostics.reserve(desc.boundary_diagnostics.size());
        for (const std::string& diagnostic : desc.boundary_diagnostics) {
            diagnostics.push_back(json_string(diagnostic));
        }
        parts.push_back("\"boundary_diagnostics\":" +
                        join_json_array(diagnostics, pretty, depth + 1));
    }

    std::ostringstream out;
    out << "{";
    if (pretty) {
        out << "\n";
    }
    for (std::size_t i = 0; i != parts.size(); ++i) {
        if (pretty) {
            out << indent(depth + 1);
        }
        out << parts[i];
        if (i + 1 != parts.size()) {
            out << ",";
        }
        if (pretty) {
            out << "\n";
        }
    }
    if (pretty) {
        out << indent(depth);
    }
    out << "}";
    return out.str();
}
}  // namespace detail

template <typename T>
abi_spec_table describe_boundary_type() {
    detail::abi_builder builder(description_mode::boundary);
    return builder.describe_root_type<^^T>();
}

template <typename T>
abi_spec_table describe_native_type() {
    detail::abi_builder builder(description_mode::native);
    return builder.describe_root_type<^^T>();
}

template <typename FunctionType>
abi_spec_table describe_function_signature() {
    detail::abi_builder builder(description_mode::boundary);
    return builder.describe_root_function_signature<FunctionType>();
}

template <meta::info Reflection>
abi_spec_table describe_function() {
    detail::abi_builder builder(description_mode::boundary);
    return builder.describe_root_function<Reflection>();
}

inline std::string to_json(const abi_spec_table& table, bool pretty) {
    std::vector<std::string> type_entries;
    type_entries.reserve(table.types.size());
    for (const type_description& type : table.types) {
        type_entries.push_back(detail::serialize_type(type, pretty, 1));
    }

    std::vector<std::string> function_entries;
    function_entries.reserve(table.functions.size());
    for (const function_description& function : table.functions) {
        function_entries.push_back(detail::serialize_function(function, pretty, 1));
    }

    std::vector<std::string> parts;
    parts.push_back("\"schema_version\":" + std::to_string(table.schema_version));
    parts.push_back("\"description_mode\":" +
                    detail::json_string(detail::to_string(table.mode)));
    if (!table.root_type_id.empty()) {
        parts.push_back("\"root_type_id\":" + detail::json_string(table.root_type_id));
    }
    if (!table.root_function_id.empty()) {
        parts.push_back("\"root_function_id\":" +
                        detail::json_string(table.root_function_id));
    }
    parts.push_back("\"types\":" + detail::join_json_array(type_entries, pretty, 1));
    parts.push_back("\"functions\":" +
                    detail::join_json_array(function_entries, pretty, 1));

    std::ostringstream out;
    out << "{";
    if (pretty) {
        out << "\n";
    }
    for (std::size_t i = 0; i != parts.size(); ++i) {
        if (pretty) {
            out << detail::indent(1);
        }
        out << parts[i];
        if (i + 1 != parts.size()) {
            out << ",";
        }
        if (pretty) {
            out << "\n";
        }
    }
    if (pretty) {
        out << "}";
    } else {
        out << "}";
    }
    return out.str();
}

inline std::string to_pretty_json(const abi_spec_table& table) {
    return to_json(table, true);
}

inline std::string to_canonical_json(const abi_spec_table& table) {
    return to_json(table, false);
}

inline std::uint64_t hash_canonical_json(std::string_view json) {
    return detail::runtime_fnv1a_64(json);
}

inline std::uint64_t hash_canonical_json(const abi_spec_table& table) {
    return hash_canonical_json(to_canonical_json(table));
}

inline boundary_check_result boundary_check(const abi_spec_table& table) {
    boundary_check_result result;
    if (!table.root_function_id.empty()) {
        const auto found = std::find_if(
            table.functions.begin(), table.functions.end(),
            [&](const function_description& function) {
                return function.id == table.root_function_id;
            });
        if (found != table.functions.end()) {
            result.accepted = found->boundary_callable;
            result.diagnostics = found->boundary_diagnostics;
            return result;
        }
    }

    if (!table.root_type_id.empty()) {
        const auto found = std::find_if(
            table.types.begin(), table.types.end(),
            [&](const type_description& type) {
                return type.id == table.root_type_id;
            });
        if (found != table.types.end()) {
            result.accepted = found->boundary_transportable;
            if (!found->boundary_diagnostic.empty()) {
                result.diagnostics.push_back(found->boundary_diagnostic);
            }
            return result;
        }
    }

    result.diagnostics.push_back("ABI table does not identify a root type or function");
    return result;
}

template <typename T>
boundary_check_result boundary_check() {
    return boundary_check(describe_boundary_type<T>());
}

template <typename FunctionType>
boundary_check_result boundary_check_function_signature() {
    return boundary_check(describe_function_signature<FunctionType>());
}

template <meta::info Reflection>
boundary_check_result boundary_check_function() {
    return boundary_check(describe_function<Reflection>());
}
}  // namespace beman::stable_abi

#endif
