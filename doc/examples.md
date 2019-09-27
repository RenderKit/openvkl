Examples
========

Open VKL ships with simple tutorial applications demonstrating the basic
usage of the API, as well as full renderers showing recommended usage.

Tutorials
---------

Simple tutorials can be found in the `examples/` directory. These are:

* `vklTutorial.c` : usage of the C API
* `vklTutorial.[cpp,ispc]` : combined usage of the C and ISPC APIs

Interactive examples
--------------------

Open VKL also ships with an interactive example application, `vklExamples`. This
interactive viewer demonstrates multiple example renderers including a path
tracer, isosurface renderer (using hit iterators), and ray marcher. The viewer
UI supports switching between renderers interactively.

Each renderer has both a C++ and ISPC implementation showing recommended API
usage. These implmentations are available in the
`examples/interactive/renderers/` directory.
