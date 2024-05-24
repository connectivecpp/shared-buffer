# Shared Buffer, Header-Only C++ 20 Reference Counted Byte Buffer Classes

#### Unit Test and Documentation Generation Workflow Status

![GH Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/connectivecpp/shared-buffer/build_run_unit_test_cmake.yml?branch=main&label=GH%20Actions%20build,%20unit%20tests%20on%20main)

![GH Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/connectivecpp/shared-buffer/build_run_unit_test_cmake.yml?branch=develop&label=GH%20Actions%20build,%20unit%20tests%20on%20develop)

![GH Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/connectivecpp/shared-buffer/gen_docs.yml?branch=main&label=GH%20Actions%20generate%20docs)

![GH Tag](https://img.shields.io/github/v/tag/connectivecpp/shared-buffer?label=GH%20tag)

## Overview

The `shared_buffer` classes are reference counted `std::byte` buffer classes useful for asynchronous networking. In particular, the Asio asynchronous networking library requires a buffer to be kept alive and valid until the outstanding IO operation (e.g. a network write) is completed. A straightforward and idiomatic way to achieve this is by using reference counted buffers.

There are two classes - `const_shared_buffer` for outgoing buffers (which should not be modified), and `mutable_shared_buffer` for incoming buffers (mutable and expandable as data arrives). There are efficient (move) operations for creating a `const_shared_buffer` from a `mutable_shared_buffer`, which allows the use case of creating a message and serializing its contents, then sending it out over the network.

While internally all data is kept in `std::byte` buffers, convenience methods are provided for converting between traditional buffer types (such as `char *` or `unsigned char*` or similar).

## Generated Documentation

The generated Doxygen documentation for `shared_buffer` is [here](https://connectivecpp.github.io/shared-buffer/).

## Dependencies

The `shared_buffer` header file does not have any third-party dependencies. It uses C++ standard library headers only. The unit test code does have dependencies as noted below.

## C++ Standard

`shared_buffer`  uses C++ 20 features, including the "spaceship" operator (`<=>`), `std::span`, and `concepts` / `requires`.

## Supported Compilers

Continuous integration workflows build and unit test on g++ (through Ubuntu), MSVC (through Windows), and clang (through macOS).

## Unit Test Dependencies

The unit test code uses [Catch2](https://github.com/catchorg/Catch2). If the `SHARED_BUFFER_BUILD_TESTS` flag is provided to Cmake (see commands below) the Cmake configure / generate will download the Catch2 library as appropriate using the [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) dependency manager. If Catch2 (v3 or greater) is already installed using a different package manager (such as Conan or vcpkg), the `CPM_USE_LOCAL_PACKAGES` variable can be set which results in `find_package` being attempted. Note that v3 (or later) of Catch2 is required.

The unit test uses utilities from Connective C++'s [utility-rack](https://github.com/connectivecpp/utility-rack).

Specific version (or branch) specs for the dependenies are in `test/CMakeLists.txt`.

## Build and Run Unit Tests

To build and run the unit test program:

First clone the `shared-buffer` repository, then create a build directory in parallel to the `shared-buffer` directory (this is called "out of source" builds, which is recommended), then `cd` (change directory) into the build directory. The CMake commands:

```
cmake -D SHARED_BUFFER_BUILD_TESTS:BOOL=ON ../shared-buffer

cmake --build .

ctest
```

For additional test output, run the unit test individually, for example:

```
test/shared_buffer_test -s
```

The example can be built by adding `-D SHARED_BUFFER_BUILD_EXAMPLES:BOOL=ON` to the CMake configure / generate step.

