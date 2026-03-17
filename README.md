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

The prototype has been ported into library headers under `include/beman/stable_abi/`.
The main entry point is:

```cpp
#include <beman/stable_abi/stable_abi.hpp>
```

The code currently assumes a compiler that provides `<experimental/meta>` and the older reflection spellings used by the original prototype.
Build validation against a current reflection compiler is the next integration step.

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
* a matching standard library with `<meta>` support

The default presets keep tests and examples disabled:

* `BEMAN_STABLE_ABI_BUILD_TESTS=OFF`
* `BEMAN_STABLE_ABI_BUILD_EXAMPLES=OFF`

For the local Bloomberg Clang/P2996 toolchain checked out on this machine, use:

```bash
cmake --workflow --preset p2996-debug
```

That preset enables both `-freflection` and `-fexpansion-statements`, builds the
example, and runs the current compile-time ABI hash test.

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
