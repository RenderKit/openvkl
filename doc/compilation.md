Packages
========

Precompiled Open VKL packages for Linux, macOS, and Windows are available via
[Open VKL GitHub releases](https://github.com/openvkl/openvkl/releases).
Packages with "sycl" in the name include support for both x86 CPUs and Intel®
GPUs, while the other packages only include x86 CPU support. Open VKL can be
compiled from source (needed for ARM platforms) following the compilation
instructions below.

GPU Runtime Requirements
========================

To run Open VKL on Intel® GPUs you will need to first have drivers installed on
your system.

### GPU drivers on Linux

Install the latest GPGPU drivers for your Intel® GPU from:
https://dgpu-docs.intel.com/. Follow the driver installation instructions for
your graphics card.

### GPU drivers on Windows

Install the latest GPGPU drivers for your Intel® GPU from:
https://www.intel.com/content/www/us/en/download/785597/intel-arc-iris-xe-graphics-windows.html.
Follow the driver installation instructions for your graphics card.

Building Open VKL from source
=============================

The latest Open VKL sources are always available at the [Open VKL GitHub
repository](http://github.com/OpenVKL/openvkl). The default `master` branch
should always point to the latest tested bugfix release.

Prerequisites
-------------

Open VKL currently supports Linux, Mac OS\ X, and Windows on CPU; and Linux and
Windows on Intel® GPUs. Before you can build Open VKL you need the following
prerequisites:

-   You can clone the latest Open VKL sources via:

        git clone https://github.com/openvkl/openvkl.git

-   To build Open VKL you need [CMake](http://www.cmake.org), any form of C++11
    compiler (we recommend using GCC, but also support Clang and MSVC), and
    standard Linux development tools. To build the examples, you should also
    have some version of OpenGL.

-   Additionally you require a copy of the [Intel® Implicit SPMD Program
    Compiler (Intel® ISPC)](http://ispc.github.io), version 1.18.0 or later.
    Please obtain a release of ISPC from the [ISPC downloads
    page](https://ispc.github.io/downloads.html).

-   Open VKL depends on the Intel RenderKit common library, rkcommon. rkcommon
    is available at the [rkcommon GitHub
    repository](https://github.com/ospray/rkcommon).

-   Open VKL depends on Embree, which is available at the [Embree GitHub
    repository](https://github.com/embree/embree).

Depending on your Linux distribution you can install these dependencies using
`yum` or `apt-get`. Some of these packages might already be installed or might
have slightly different names.

### GPU-specific Prerequisites

In addition, if you would like to build Open VKL for Intel® GPUs on Linux or
Windows, you need the following additional prerequisites:

-   [CMake](http://www.cmake.org) version 3.25.3 or higher

-   Download the [oneAPI DPC++ Compiler
    2023-09-22](https://github.com/intel/llvm/releases/tag/nightly-2023-09-22);
    please note this specific version has been validated and used in our
    releases.

    -   On Linux, the compiler can be simply extracted, then set up using the
        following commands in bash (where `path_to_dpcpp_compiler` should point
        to the root directory of unpacked package):

        ```
        export SYCL_BUNDLE_ROOT=path_to_dpcpp_compiler
        export PATH=$SYCL_BUNDLE_ROOT/bin:$PATH
        export CPATH=$SYCL_BUNDLE_ROOT/include:$CPATH
        export LIBRARY_PATH=$SYCL_BUNDLE_ROOT/lib:$LIBRARY_PATH
        export LD_LIBRARY_PATH=$SYCL_BUNDLE_ROOT/lib:$LD_LIBRARY_PATH
        export LD_LIBRARY_PATH=$SYCL_BUNDLE_ROOT/linux/lib/x64:$LD_LIBRARY_PATH
        ```

    -   On Windows, you will also need an installed version of Visual Studio
        that supports the C++17 standard, e.g. Visual Studio 2019. Then,
        download and unpack the DPC++ compiler package and open the "x64 Native
        Tools Command Prompt" of Visual Studio. Execute the following lines to
        properly configure the environment to use the oneAPI DPC++ compiler
        (where `path_to_dpcpp_compiler` should point to the root directory of
        unpacked package):

        ```
        set "DPCPP_DIR=path_to_dpcpp_compiler"
        set "PATH=%DPCPP_DIR%\bin;%PATH%"
        set "PATH=%DPCPP_DIR%\lib;%PATH%"
        set "CPATH=%DPCPP_DIR%\include;%CPATH%"
        set "INCLUDE=%DPCPP_DIR%\include;%INCLUDE%"
        set "LIB=%DPCPP_DIR%\lib;%LIB%"
        ```

CMake Superbuild
----------------

For convenience, Open VKL provides a CMake Superbuild script which will pull
down Open VKL's dependencies and build Open VKL itself. The result is an install
directory, with each dependency in its own directory.

Run with:

```bash
mkdir build
cd build
cmake [<VKL_ROOT>/superbuild]
cmake --build .
```

If you wish to enable GPU support, additional flags must be passed to the
superbuild. On Linux:

    ```
    export CC=clang
    export CXX=clang++

    cmake -D OPENVKL_EXTRA_OPTIONS="-DOPENVKL_ENABLE_DEVICE_GPU=ON" \
      [<VKL_ROOT>/superbuild]
    ```

And on Windows:

    ```
    cmake -L -G Ninja \
      -D CMAKE_CXX_COMPILER=clang-cl -D CMAKE_C_COMPILER=clang-cl \
      -D OPENVKL_EXTRA_OPTIONS="-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++  -DOPENVKL_ENABLE_DEVICE_GPU=ON" \
       [<VKL_ROOT>/superbuild]
    ```

The resulting `install` directory (or the one set with `CMAKE_INSTALL_PREFIX`)
will have everything in it, with one subdirectory per dependency.

CMake options to note (all have sensible defaults):

- `CMAKE_INSTALL_PREFIX` will be the root directory where everything gets
  installed.
- `BUILD_JOBS` sets the number given to `make -j` for parallel builds.
- `INSTALL_IN_SEPARATE_DIRECTORIES` toggles installation of all libraries in
  separate or the same directory.
- `BUILD_TBB_FROM_SOURCE` specifies whether TBB should be built from source or
   the releases on Gitub should be used. This must be ON when compiling for ARM.
- `OPENVKL_ENABLE_DEVICE_GPU` specifies if GPU support should be enabled. Note
  this defaults to `OFF`.
-
For the full set of options, run `ccmake [<VKL_ROOT>/superbuild]`.

Standard CMake build
--------------------

Assuming the above prerequisites are all fulfilled, building Open VKL through
CMake is easy:

-   Create a build directory, and go into it

        mkdir openvkl/build
        cd openvkl/build

    (We do recommend having separate build directories for different
    configurations such as release, debug, etc.).

-   The compiler CMake will use will default to whatever the `CC` and `CXX`
    environment variables point to. Should you want to specify a different
    compiler, run cmake manually while specifying the desired compiler. The
    default compiler on most linux machines is `gcc`, but it can be pointed to
    `clang` instead by executing the following:

        cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ..

    CMake will now use Clang instead of GCC. If you are ok with using the
    default compiler on your system, then simply skip this step. Note that the
    compiler variables cannot be changed after the first `cmake` or `ccmake`
    run.

-   Open the CMake configuration dialog

        ccmake ..

-   Make sure to properly set build mode and enable the components you need,
    etc.; then type 'c'onfigure and 'g'enerate. When back on the command prompt,
    build it using

        make

-   You should now have `libopenvkl.so` as well as the tutorial / example
    applications.
