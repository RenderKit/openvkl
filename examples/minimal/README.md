# Intel® Open Volume Kernel Library: Minimal Examples

This directory contains a sequence of minimal code examples that make use of
Open VKL. These examples are designed to be read and understood in sequence;
each example builds upon the previous one.

The examples provided are:

- `minimal_01.cpp`: prerequisite code infrastructure for managing a framebuffer,
  using a transfer function, and drawing the frame buffer to the terminal.
- `minimal_02.cpp`: initializing Open VKL
- `minimal_03.cpp`: instantiating a VKL volume, sampler, and rendering a slice
- `minimal_04.cpp`: changing volume types
- `minimal_05.cpp`: creating a ray marching volume renderer
- `minimal_06.cpp`: creating an isosurface renderer

For more complex examples, see the `vklExamples` application and corresponding
code.
