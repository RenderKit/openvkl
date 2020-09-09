Building Open VKL from source
=============================

The latest Open VKL sources are always available at the [Open VKL GitHub
repository](http://github.com/OpenVKL/openvkl). The default `master` branch
should always point to the latest tested bugfix release.

Prerequisites
-------------

Open VKL currently supports Linux, Mac OS\ X, and Windows. In addition, before
you can build Open VKL you need the following prerequisites:

-   You can clone the latest Open VKL sources via:

        git clone https://github.com/openvkl/openvkl.git

-   To build Open VKL you need [CMake](http://www.cmake.org), any form of C++11
    compiler (we recommend using GCC, but also support Clang and MSVC), and
    standard Linux development tools. To build the examples, you should also
    have some version of OpenGL.

-   Additionally you require a copy of the [Intel® SPMD Program Compiler
    (ISPC)](http://ispc.github.io), version 1.14.1 or later. Please obtain a
    release of ISPC from the [ISPC downloads
    page](https://ispc.github.io/downloads.html).

-   Open VKL depends on the Intel RenderKit common library, rkcommon. rkcommon is
    available at the [rkcommon GitHub
    repository](https://github.com/ospray/rkcommon).

-   Open VKL depends on Embree, which is available at the [Embree GitHub
    repository](https://github.com/embree/embree).

Depending on your Linux distribution you can install these dependencies using
`yum` or `apt-get`. Some of these packages might already be installed or might
have slightly different names.

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

The resulting `install` directory (or the one set with `CMAKE_INSTALL_PREFIX`)
will have everything in it, with one subdirectory per dependency.

CMake options to note (all have sensible defaults):

- `CMAKE_INSTALL_PREFIX` will be the root directory where everything gets
  installed.
- `BUILD_JOBS` sets the number given to `make -j` for parallel builds.
- `INSTALL_IN_SEPARATE_DIRECTORIES` toggles installation of all libraries in
  separate or the same directory.

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

-   The compiler CMake will use will default to whatever the `CC` and
    `CXX` environment variables point to. Should you want to specify a
    different compiler, run cmake manually while specifying the desired
    compiler. The default compiler on most linux machines is `gcc`, but
    it can be pointed to `clang` instead by executing the following:

        cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ..

    CMake will now use Clang instead of GCC. If you are ok with using
    the default compiler on your system, then simply skip this step.
    Note that the compiler variables cannot be changed after the first
    `cmake` or `ccmake` run.

-   Open the CMake configuration dialog

        ccmake ..

-   Make sure to properly set build mode and enable the components you
    need, etc.; then type 'c'onfigure and 'g'enerate. When back on the
    command prompt, build it using

        make

-   You should now have `libopenvkl.so` as well as the tutorial / example
    applications.
