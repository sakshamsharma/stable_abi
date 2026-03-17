# Development

## Configure and Build the Project Using CMake Presets

The simplest way to configure and build the project is to use
[CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html).
You can list the available workflow presets with:

```shell
cmake --list-presets=workflow
```

Example:

```shell
cmake --workflow --preset gcc-debug
```

## Configure and Build Manually

If the presets are not suitable for your use case, a traditional CMake invocation
provides more configurability.

```bash
cmake \
  -B build \
  -S . \
  -DCMAKE_CXX_STANDARD=26
cmake --build build
ctest --test-dir build
```

> [!IMPORTANT]
>
> Beman projects are passive projects, so you need to specify the C++ version via
> `CMAKE_CXX_STANDARD` when manually configuring the project.

## Dependency Management

### FetchContent

Instead of installing project dependencies manually, you can configure
`beman.stable_abi` to fetch them via CMake FetchContent:

```shell
cmake \
  -B build \
  -S . \
  -DCMAKE_CXX_STANDARD=26 \
  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=./infra/cmake/use-fetch-content.cmake
cmake --build build
ctest --test-dir build
```

The file `./lockfile.json` configures the dependency versions acquired by FetchContent.

## Project-specific Configure Arguments

Project-specific options are prefixed with `BEMAN_STABLE_ABI`.
You can inspect them with:

```bash
cmake -LH -S . -B build | grep "BEMAN_STABLE_ABI" -C 2
```

### `BEMAN_STABLE_ABI_BUILD_TESTS`

Enable building tests and test infrastructure. Default: `OFF`.
Values: `{ ON, OFF }`.

### `BEMAN_STABLE_ABI_BUILD_EXAMPLES`

Enable building examples. Default: `OFF`.
Values: `{ ON, OFF }`.
