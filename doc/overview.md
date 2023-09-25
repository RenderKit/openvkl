Overview
========

Intel® Open Volume Kernel Library (Intel® Open VKL) is a collection of
high-performance volume computation kernels, developed at Intel. The target
users of Open VKL are graphics application engineers who want to improve the
performance of their volume rendering applications by leveraging Open VKL’s
performance-optimized kernels, which include volume traversal and sampling
functionality for a variety of volumetric data formats. Open VKL supports x86
CPUs under Linux, macOS, and Windows; ARM CPUs on macOS; as well as Intel® GPUs
under Linux and Windows (currently in beta).

Open VKL contains kernels optimized for the latest x86 processors with support
for SSE, AVX, AVX2, and AVX-512 instructions, and for ARM processors with
support for NEON instructions. Open VKL supports Intel GPUs based on the Xe HPG
microarchitecture (Intel® Arc™ GPU) under Linux and Windows and Xe HPC
microarchitecture (Intel® Data Center GPU Flex Series and Intel® Data Center GPU
Max Series) under Linux. Intel GPU support leverages the
[SYCL](https://www.khronos.org/sycl/) open standard programming language; SYCL
allows one to write C++ code that can be run on various devices, such as CPUs
and GPUs. Open VKL is part of the [Intel® oneAPI Rendering
Toolkit](https://software.intel.com/en-us/rendering-framework) and is released
under the permissive [Apache 2.0
license](http://www.apache.org/licenses/LICENSE-2.0).

Open VKL provides a C-based API on CPU and GPU, and also supports applications
written with the Intel® Implicit SPMD Program Compiler (Intel® ISPC) for CPU by
also providing an ISPC interface to the core volume algorithms. This makes it
possible to write a renderer in ISPC that automatically vectorizes and leverages
SSE, AVX, AVX2, AVX-512, and NEON instructions. ISPC also supports runtime code
selection, thus ISPC will select the best code path for your application.

In addition to the volume kernels, Open VKL provides tutorials and example
renderers to demonstrate how to best use the Open VKL API.
