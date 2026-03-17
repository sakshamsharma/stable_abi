# beman.stable_abi: ABI Comparison with Reflection

<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

<!-- markdownlint-disable-next-line line-length -->
![Library Status](https://raw.githubusercontent.com/bemanproject/beman/refs/heads/main/images/badges/beman_badge-beman_library_under_development.svg) ![Standard Target](https://github.com/bemanproject/beman/blob/main/images/badges/cpp29.svg)

`beman.stable_abi` is a Beman candidate library for experimenting with ABI comparison techniques built on C++ static reflection.
It is based on the ideas in [`20240117 - ABI comparison with reflection.pdf`](./20240117%20-%20ABI%20comparison%20with%20reflection.pdf) and bootstrapped from the Beman exemplar project.

**Implements**: the exploratory ABI hashing and ABI description ideas described in `P3095R1`, "ABI comparison with reflection".

**Status**: [Under development and not yet ready for production use.](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#under-development-and-not-yet-ready-for-production-use)

## License

`beman.stable_abi` is licensed under the Apache License v2.0 with LLVM Exceptions.

## Usage

This repository is intentionally implementation-empty in its first commit boundary.
The current public header only reserves the library namespace and documents the project scope.

## Scope

The first implementation pass is expected to cover:

* recursive ABI hashing for aggregate and class layouts
* hashing configuration types controlling name and indirection sensitivity
* examples built around the `Order`, `MyList<T>`, and `OrderBook` style layouts from the original prototype
* tests that compare structural changes against hash changes

Deferred topics include:

* vtable and virtual-function ABI hashing
* full serialized ABI descriptions for message parsing
* enum-specific policies
* any `std::hash` integration that depends on unresolved portability questions

## Dependencies

### Build Environment

This project currently requires:

* a C++ compiler with experimental static reflection support
* CMake 3.30 or later
* GoogleTest for future tests

The empty scaffold keeps tests and examples disabled by default:

* `BEMAN_STABLE_ABI_BUILD_TESTS=OFF`
* `BEMAN_STABLE_ABI_BUILD_EXAMPLES=OFF`

### Supported Platforms

Compiler support is still to be validated against current post-P2996 reflection implementations.
The original prototype targeted an older experimental reflection compiler and will need adaptation before this matrix can be made precise.

## Development

See the [Contributing Guidelines](CONTRIBUTING.md).

## Build

You can configure the empty scaffold with a workflow preset:

```bash
cmake --workflow --preset gcc-release
```

To list available workflow presets:

```bash
cmake --list-presets=workflow
```

For details on building `beman.stable_abi` without a preset, refer to the
[Contributing Guidelines](CONTRIBUTING.md).
