Overview
========

Intel® Open Volume Kernel Library (Intel® Open VKL) is a collection of
high-performance volume computation kernels, developed at Intel. The target
users of Open VKL are graphics application engineers who want to improve the
performance of their volume rendering applications by leveraging Open VKL’s
performance-optimized kernels, which include volume traversal and sampling
functionality for a variety of volumetric data formats. The kernels are
optimized for the latest Intel® processors with support for SSE, AVX, AVX2, and
AVX-512 instructions. Open VKL is part of the [Intel® oneAPI Rendering
Toolkit](https://software.intel.com/en-us/rendering-framework) and is released
under the permissive [Apache 2.0
license](http://www.apache.org/licenses/LICENSE-2.0).


Open VKL provides a C API, and also supports applications written with the
Intel® Implicit SPMD Program Compiler (Intel® ISPC) by also providing an ISPC
interface to the core volume algorithms. This makes it possible to write a
renderer in ISPC that automatically vectorizes and leverages SSE, AVX, AVX2, and
AVX-512 instructions. ISPC also supports runtime code selection, thus ISPC will
select the best code path for your application.

In addition to the volume kernels, Open VKL provides tutorials and example
renderers to demonstrate how to best use the Open VKL API.
