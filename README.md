# Intel® Open Volume Kernel Library

This is release v0.7.0 of Intel® Open VKL. For changes and new features
see the [changelog](CHANGELOG.md). Visit http://www.openvkl.org for more
information.

# Overview

Intel® Open Volume Kernel Library (Intel® Open VKL) is a collection of
high-performance volume computation kernels, developed at Intel. The
target users of Open VKL are graphics application engineers who want to
improve the performance of their volume rendering applications by
leveraging Open VKL’s performance-optimized kernels, which include
volume traversal and sampling functionality for a variety of volumetric
data formats. The kernels are optimized for the latest Intel® processors
with support for SSE, AVX, AVX2, and AVX-512 instructions. Open VKL is
part of the [Intel® oneAPI Rendering
Toolkit](https://software.intel.com/en-us/rendering-framework) and is
released under the permissive [Apache 2.0
license](http://www.apache.org/licenses/LICENSE-2.0).

Open VKL provides a C API, and also supports applications written with
the Intel® SPMD Program Compiler (ISPC) by also providing an ISPC
interface to the core volume algorithms. This makes it possible to write
a renderer in ISPC that automatically vectorizes and leverages SSE, AVX,
AVX2, and AVX-512 instructions. ISPC also supports runtime code
selection, thus ISPC will select the best code path for your
application.

In addition to the volume kernels, Open VKL provides tutorials and
example renderers to demonstrate how to best use the Open VKL API.

## Version History

### Open VKL 0.7.0 (alpha)

  - Initial public alpha release, with support for structured,
    unstructured, and AMR volumes.

## Support and Contact

Open VKL is under active development, and though we do our best to
guarantee stable release versions a certain number of bugs,
as-yet-missing features, inconsistencies, or any other issues are still
possible. Should you find any such issues please report them immediately
via [Open VKL’s GitHub Issue
Tracker](https://github.com/OpenVKL/openvkl/issues) (or, if you should
happen to have a fix for it, you can also send us a pull request); for
missing features please contact us via email at
<openvkl@googlegroups.com>.

Join our [mailing
list](https://groups.google.com/forum/#!forum/openvkl-announce/join) to
receive release announcements and major news regarding Open VKL.

# Intel® Open VKL API

To access the Open VKL API you first need to include the Open VKL
header. For C99 or C++:

``` cpp
#include <openvkl/openvkl.h>
```

For the Intel SPMD Program Compiler (ISPC):

``` cpp
#include <openvkl/openvkl.isph>
```

This documentation will discuss the C99/C++ API. The ISPC version has
the same functionality and flavor. Looking at the headers, the
`vklTutorialISPC` example, and this documentation should be enough to
figure it out.

## Initialization and shutdown

To use the API, one of the implemented backends must be loaded.
Currently the only one that exists is the ISPC driver. ISPC in the name
here just refers to the implementation language – it can also be used
from the C99/C++ APIs. To load the module that implements the ISPC
driver:

``` cpp
vklLoadModule("ispc_driver");
```

The driver then needs to be selected:

``` cpp
VKLDriver driver = vklNewDriver("ispc");
vklCommitDriver(driver);
vklSetCurrentDriver(driver);
```

Open VKL provides vector-wide versions for several APIs. To determine
the native vector width for the given driver, call:

``` cpp
int width = vklGetNativeSIMDWidth();
```

When the application is finished with Open VKL or shutting down, call
the shutdown function:

``` cpp
vklShutdown();
```

## Basic data types

Open VKL defines 3-component vectors of integer and vector types:

``` cpp
typedef struct
{
  int x, y, z;
} vkl_vec3i;

typedef struct
{
  float x, y, z;
} vkl_vec3f;
```

Vector versions of these are also defined in structure-of-array format
for 4, 8, and 16 wide types.

``` cpp
  typedef struct
  {
    float x[WIDTH];
    float y[WIDTH];
    float z[WIDTH];
  } vkl_vvec3f##WIDTH;

  typedef struct
  {
    float lower[WIDTH], upper[WIDTH];
  } vkl_vrange1f##WIDTH;
```

1-D range and 3-D ranges are defined as ranges and boxes, with no vector
versions:

``` cpp
typedef struct
{
  float lower, upper;
} vkl_range1f;

typedef struct
{
  vkl_vec3f lower, upper;
} vkl_box3f;
```

## Object model

Objects in Open VKL are exposed to the APIs as handles with internal
reference counting for lifetime determination. Objects are created with
particular type’s `vklNew...` API entry point. For example, `vklNewData`
and `vklNewVolume`.

In general, modifiable parameters to objects are modified using
`vklSet...` functions based on the type of the parameter being set. The
parameter name is passed as a string. Below are all variants of
`vklSet...`.

``` cpp
void vklSetBool(VKLObject object, const char *name, int b);
void vklSetFloat(VKLObject object, const char *name, float x);
void vklSetVec3f(VKLObject object, const char *name, float x, float y, float z);
void vklSetInt(VKLObject object, const char *name, int x);
void vklSetVec3i(VKLObject object, const char *name, int x, int y, int z);
void vklSetData(VKLObject object, const char *name, VKLData data);
void vklSetString(VKLObject object, const char *name, const char *s);
void vklSetVoidPtr(VKLObject object, const char *name, void *v);
```

The exception to this rule is the `VKLValueSelector` object (described
in the iterators section below), which has object-specific set methods.
The reason for this is to align the C99/C++ API with the ISPC API, which
can’t use a parameter method due to language limitations.

After parameters have been set, `vklCommit` must be called on the object
to make them take effect.

Open VKL uses reference counting to manage the lifetime of all objects.
Therefore one cannot explicitly “delete” any object. Instead, one can
indicate the application does not need or will not access the given
object anymore by calling

``` cpp
void vklRelease(VKLObject);
```

This decreases the object’s reference count. If the count reaches `0`
the object will automatically be deleted.

## Managed data

Large data is passed to Open VKL via a `VKLData` handle created with
`vklNewData`:

``` cpp
VKLData vklNewData(size_t numItems,
                   VKLDataType dataType,
                   const void *source,
                   VKLDataCreationFlags dataCreationFlags);
```

Types accepted are listed in `VKLDataType.h`; basic types (`UCHAR`,
`INT`, `UINT`, `LONG`, `ULONG`) exist as both scalar and chunked
formats. The types accepted vary per volume at the moment; read the
volume section below for specifics.

Data objects can be created as Open VKL owned (`dataCreationFlags =
VKL_DATA_DEFAULT`), in which the library will make a copy of the data
for its use, or shared (`dataCreationFlags = VKL_DATA_SHARED_BUFFER`),
which will try to use the passed pointer for usage. The library is
allowed to copy data when a volume is committed.

As with other object types, when data objects are no longer needed they
should be released via `vklRelease`.

## Volume types

Open VKL currently supports structured regular volumes (with user
specified spacing on axes); unstructured volumes with tetrahedral,
wedge, pyramid, and hexaderal primitive types; and adaptive mesh
refinement (AMR) volumes. These volumes are created with `vlkNewVolume`
with the appropriate type string.

In addition to the usual `vklSet...()` and `vklCommit()` APIs, the
volume bounding box can be queried:

``` cpp
vkl_box3f vklGetBoundingBox(VKLVolume volume);
```

### Structured Volume

Structured volumes only need to store the values of the samples, because
their addresses in memory can be easily computed from a 3D position. A
common type of structured volumes are regular grids. Structured grids
are created by passing a type string of `"structured_regular"` to
`vklNewVolume`.

The parameters understood by structured volumes are summarized in the
table below.

| Type  | Name        |    Default    | Description                                        |
| :---- | :---------- | :-----------: | :------------------------------------------------- |
| vec3i | dimensions  |               | number of voxels in each dimension \((x, y, z)\)   |
| data  | voxelData   |               | VKLData object of voxel data, supported types are: |
|       |             |               | `VKL_UCHAR`                                        |
|       |             |               | `VKL_SHORT`                                        |
|       |             |               | `VKL_USHORT`                                       |
|       |             |               | `VKL_FLOAT`                                        |
|       |             |               | `VKL_DOUBLE`                                       |
| vec3f | gridOrigin  | \((0, 0, 0)\) | origin of the grid in world-space                  |
| vec3f | gridSpacing | \((1, 1, 1)\) | size of the grid cells in world-space              |

Additional configuration parameters for structured volumes.

The dimensions for structured volumes are in terms units vertices, not
cells. For example, a volume with dimensions \((x, y, z)\) will have
\((x-1, y-1, z-1)\) cells in each dimension. Voxel data provided is
assumed vertex-centered, so \(x*y*z\) values must be provided.

### Adaptive Mesh Refinement (AMR) Volume

AMR volumes are specified as a list of blocks, which exist at levels of
refinement in potentially overlapping regions. Blocks exist in a tree
structure, with coarser refinement level blocks containing finer blocks.
The cell width is equal for all blocks at the same refinement level,
though blocks at a coarser level have a larger cell width than finer
levels.

There can be any number of refinement levels and any number of blocks at
any level of refinement. An AMR volume type is created by passing the
type string `"amr"` to `vklNewVolume`.

Blocks are defined by four parameters: their bounds, the refinement
level in which they reside, the cell widths for each refinement level,
and the scalar data contained within each block.

Note that cell widths are defined *per refinement level*, not per block.

| Type           | Name            |           Default | Description                                                       |
| :------------- | :-------------- | ----------------: | :---------------------------------------------------------------- |
| `VKLAMRMethod` | method          | `VKL_AMR_CURRENT` | `VKLAMRMethod` sampling method. Supported methods are:            |
|                |                 |                   | `VKL_AMR_CURRENT`                                                 |
|                |                 |                   | `VKL_AMR_FINEST`                                                  |
|                |                 |                   | `VKL_AMR_OCTANT`                                                  |
| box3f\[\]      | block.bounds    |              NULL | \[data\] array of bounds for each AMR block                       |
| int\[\]        | block.level     |              NULL | array of each block’s refinement level                            |
| float\[\]      | block.cellWidth |              NULL | array of each block’s cell width                                  |
| VKLData\[\]    | block.data      |              NULL | \[data\] array of VKLData containing the actual scalar voxel data |
| vec3f          | gridOrigin      |     \((0, 0, 0)\) | origin of the grid in world-space                                 |
| vec3f          | gridSpacing     |     \((1, 1, 1)\) | size of the grid cells in world-space                             |

Additional configuration parameters for AMR volumes.

Lastly, note that the `gridOrigin` and `gridSpacing` parameters act just
like the structured volume equivalent, but they only modify the root
(coarsest level) of refinement.

In particular, Open VKL’s AMR implementation was designed to cover
Berger-Colella \[1\] and Chombo \[2\] AMR data. The `method` parameter
above determines the interpolation method used when sampling the volume.

  - `VKL_AMR_CURRENT` finds the finest refinement level at that cell and
    interpolates through this “current” level
  - `VKL_AMR_FINEST` will interpolate at the closest existing cell in
    the volume-wide finest refinement level regardless of the sample
    cell’s level
  - `VKL_AMR_OCTANT` interpolates through all available refinement
    levels at that cell. This method avoids discontinuities at
    refinement level boundaries at the cost of performance

Details and more information can be found in the publication for the
implementation \[3\].

1.  M. J. Berger, and P. Colella. “Local adaptive mesh refinement for
    shock hydrodynamics.” Journal of Computational Physics 82.1 (1989):
    64-84. DOI: 10.1016/0021-9991(89)90035-1
2.  M. Adams, P. Colella, D. T. Graves, J.N. Johnson, N.D. Keen, T. J.
    Ligocki. D. F. Martin. P.W. McCorquodale, D. Modiano. P.O. Schwartz,
    T.D. Sternberg and B. Van Straalen, Chombo Software Package for AMR
    Applications - Design Document, Lawrence Berkeley National
    Laboratory Technical Report LBNL-6616E.
3.  I. Wald, C. Brownlee, W. Usher, and A. Knoll. CPU volume rendering
    of adaptive mesh refinement data. SIGGRAPH Asia 2017 Symposium on
    Visualization on - SA ’17, 18(8), 1–8. DOI: 10.1145/3139295.3139305

### Unstructured Volumes

Unstructured volumes can have their topology and geometry freely
defined. Geometry can be composed of tetrahedral, hexahedral, wedge or
pyramid cell types. The data format used is compatible with VTK and
consists of multiple arrays: vertex positions and values, vertex
indices, cell start indices, cell types, and cell values. An
unstructured volume type is created by passing the type string
`"unstructured"` to `vklNewVolume`.

Sampled cell values can be specified either per-vertex (`vertex.value`)
or per-cell (`cell.value`). If both arrays are set, `cell.value` takes
precedence.

Similar to a mesh, each cell is formed by a group of indices into the
vertices. For each vertex, the corresponding (by array index) data value
will be used for sampling when rendering, if specified. The index order
for a tetrahedron is the same as `VTK_TETRA`: bottom triangle
counterclockwise, then the top vertex.

For hexahedral cells, each hexahedron is formed by a group of eight
indices into the vertices and data values. Vertex ordering is the same
as `VTK_HEXAHEDRON`: four bottom vertices counterclockwise, then top
four counterclockwise.

For wedge cells, each wedge is formed by a group of six indices into the
vertices and data values. Vertex ordering is the same as `VTK_WEDGE`:
three bottom vertices counterclockwise, then top three counterclockwise.

For pyramid cells, each cell is formed by a group of five indices into
the vertices and data values. Vertex ordering is the same as
`VTK_PYRAMID`: four bottom vertices counterclockwise, then the top
vertex.

To maintain VTK data compatibility an index array may be specified via
the `indexPrefixed` array that allows vertex indices to be interleaved
with cell sizes in the following format:
\(n, id_1, ..., id_n, m, id_1, ..., id_m\).

| Type                    | Name               | Default | Description                                                                                                                                             |
| :---------------------- | :----------------- | :------ | :------------------------------------------------------------------------------------------------------------------------------------------------------ |
| vec3f\[\]               | vertex.position    |         | \[data\] array of vertex positions                                                                                                                      |
| float\[\]               | vertex.value       |         | \[data\] array of vertex data values to be sampled                                                                                                      |
| uint32\[\] / uint64\[\] | index              |         | \[data\] array of indices (into the vertex array(s)) that form cells                                                                                    |
| uint32\[\] / uint64\[\] | indexPrefixed      |         | alternative \[data\] array of indices compatible to VTK, where the indices of each cell are prefixed with the number of vertices                        |
| uint32\[\] / uint64\[\] | cell               |         | \[data\] array of locations (into the index array), specifying the first index of each cell                                                             |
| float\[\]               | cell.value         |         | \[data\] array of cell data values to be sampled                                                                                                        |
| uint8\[\]               | cell.type          |         | \[data\] array of cell types (VTK compatible). Supported types are:                                                                                     |
|                         |                    |         | `VKL_TETRAHEDRON`                                                                                                                                       |
|                         |                    |         | `VKL_HEXAHEDRON`                                                                                                                                        |
|                         |                    |         | `VKL_WEDGE`                                                                                                                                             |
|                         |                    |         | `VKL_PYRAMID`                                                                                                                                           |
| bool                    | hexIterative       | false   | hexahedron interpolation method, defaults to fast non-iterative version which could have rendering inaccuracies may appear if hex is not parallelepiped |
| bool                    | precomputedNormals | true    | whether to accelerate by precomputing, at a cost of 12 bytes/face                                                                                       |

Additional configuration parameters for unstructured volumes.

## Sampling

Computing the value of a volume at an object space coordinate is done
using the sampling API. NaN is returned for probe points outside the
volume.

The scalar API just takes a volume and coordinate, and returns a float
value.

``` cpp
float vklComputeSample(VKLVolume volume, const vkl_vec3f *objectCoordinates);
```

Vector versions allow sampling at 4, 8, or 16 positions at once.
Depending on the machine type and Open VKL driver implementation, these
can give greater performance. An active lane mask `valid` is passed in
as an array of integers; set 0 for lanes to be ignored, -1 for active
lanes.

``` cpp
void vklComputeSample4(const int *valid,
                       VKLVolume volume,
                       const vkl_vvec3f4 *objectCoordinates,
                       float *samples);

void vklComputeSample8(const int *valid,
                       VKLVolume volume,
                       const vkl_vvec3f8 *objectCoordinates,
                       float *samples);

void vklComputeSample16(const int *valid,
                        VKLVolume volume,
                        const vkl_vvec3f16 *objectCoordinates,
                        float *samples);
```

All of the above sampling APIs can be used, regardless of the driver’s
native SIMD width.

## Gradients

In a very similar API to `vlkComputeSample`, `vlkComputeGradient`
queries the value gradient at an object space coordinate. Again, a
scalar API, now returning a vec3f instead of a float. NaN values are
returned for points outside the volume.

``` cpp
vkl_vec3f vklComputeGradient(VKLVolume volume,
                             const vkl_vec3f *objectCoordinates);
```

Vector versions are also provided:

``` cpp
void vklComputeGradient4(const int *valid,
                         VKLVolume volume,
                         const vkl_vvec3f4 *objectCoordinates,
                         vkl_vvec3f4 *gradients);

void vklComputeGradient8(const int *valid,
                         VKLVolume volume,
                         const vkl_vvec3f8 *objectCoordinates,
                         vkl_vvec3f8 *gradients);

void vklComputeGradient16(const int *valid,
                          VKLVolume volume,
                          const vkl_vvec3f16 *objectCoordinates,
                          vkl_vvec3f16 *gradients);
```

All of the above gradient APIs can be used, regardless of the driver’s
native SIMD width.

## Iterators

Open VKL has APIs to search for particular volume values along a ray.
Queries can be for ranges of volume values (`vklIterateInterval`) or for
particular values (`vklIterateHit`). The desired values are set in a
`VKLValueSelector`, which needs to be created, filled in with values,
and then committed.

``` cpp
VKLValueSelector vklNewValueSelector(VKLVolume volume);

void vklValueSelectorSetRanges(VKLValueSelector valueSelector,
                               size_t numRanges,
                               const vkl_range1f *ranges);

void vklValueSelectorSetValues(VKLValueSelector valueSelector,
                               size_t numValues,
                               const float *values);
```

To query an interval, a `VKLIntervalIterator` of scalar or vector width
must be initialized with `vklInitIntervalIterator`. The iterator
structure is allocated and belongs to the caller, and initialized by the
following functions.

``` cpp
void vklInitIntervalIterator(VKLIntervalIterator *iterator,
                             VKLVolume volume,
                             const vkl_vec3f *origin,
                             const vkl_vec3f *direction,
                             const vkl_range1f *tRange,
                             VKLValueSelector valueSelector);

void vklInitIntervalIterator4(const int *valid,
                              VKLIntervalIterator4 *iterator,
                              VKLVolume volume,
                              const vkl_vvec3f4 *origin,
                              const vkl_vvec3f4 *direction,
                              const vkl_vrange1f4 *tRange,
                              VKLValueSelector valueSelector);

void vklInitIntervalIterator8(const int *valid,
                              VKLIntervalIterator8 *iterator,
                              VKLVolume volume,
                              const vkl_vvec3f8 *origin,
                              const vkl_vvec3f8 *direction,
                              const vkl_vrange1f8 *tRange,
                              VKLValueSelector valueSelector);

void vklInitIntervalIterator16(const int *valid,
                               VKLIntervalIterator16 *iterator,
                               VKLVolume volume,
                               const vkl_vvec3f16 *origin,
                               const vkl_vvec3f16 *direction,
                               const vkl_vrange1f16 *tRange,
                               VKLValueSelector valueSelector);
```

Intervals can then be processed by calling `vklIterateInterval` as long
as the returned lane masks indicates that the iterator is still within
the volume:

``` cpp
int vklIterateInterval(VKLIntervalIterator *iterator,
                       VKLInterval *interval);

void vklIterateInterval4(const int *valid,
                         VKLIntervalIterator4 *iterator,
                         VKLInterval4 *interval,
                         int *result);

void vklIterateInterval8(const int *valid,
                         VKLIntervalIterator8 *iterator,
                         VKLInterval8 *interval,
                         int *result);

void vklIterateInterval16(const int *valid,
                          VKLIntervalIterator16 *iterator,
                          VKLInterval16 *interval,
                          int *result);
```

The intervals returned have a t-value range, a value range, and a
`nominalDeltaT` which is approximately the step size that should be used
to walk through the interval, if desired. The number and length of
intervals returned is volume type implementation dependent. There is
currently no way of requesting a particular splitting.

``` cpp
typedef struct
{
  vkl_range1f tRange;
  vkl_range1f valueRange;
  float nominalDeltaT;
} VKLInterval;

typedef struct
{
  vkl_vrange1f4 tRange;
  vkl_vrange1f4 valueRange;
  float nominalDeltaT[4];
} VKLInterval4;

typedef struct
{
  vkl_vrange1f8 tRange;
  vkl_vrange1f8 valueRange;
  float nominalDeltaT[8];
} VKLInterval8;

typedef struct
{
  vkl_vrange1f16 tRange;
  vkl_vrange1f16 valueRange;
  float nominalDeltaT[16];
} VKLInterval16;
```

Querying for particular values is done using a `VKLHitIterator` in much
the same fashion. This API could be used, for example, to find
isosurfaces. Again, a user allocated `VKLHitIterator` of the desired
width must be initialized:

``` cpp
void vklInitHitIterator(VKLHitIterator *iterator,
                        VKLVolume volume,
                        const vkl_vec3f *origin,
                        const vkl_vec3f *direction,
                        const vkl_range1f *tRange,
                        VKLValueSelector valueSelector);

void vklInitHitIterator4(const int *valid,
                         VKLHitIterator4 *iterator,
                         VKLVolume volume,
                         const vkl_vvec3f4 *origin,
                         const vkl_vvec3f4 *direction,
                         const vkl_vrange1f4 *tRange,
                         VKLValueSelector valueSelector);

void vklInitHitIterator8(const int *valid,
                         VKLHitIterator8 *iterator,
                         VKLVolume volume,
                         const vkl_vvec3f8 *origin,
                         const vkl_vvec3f8 *direction,
                         const vkl_vrange1f8 *tRange,
                         VKLValueSelector valueSelector);

void vklInitHitIterator16(const int *valid,
                          VKLHitIterator16 *iterator,
                          VKLVolume volume,
                          const vkl_vvec3f16 *origin,
                          const vkl_vvec3f16 *direction,
                          const vkl_vrange1f16 *tRange,
                          VKLValueSelector valueSelector);
```

Hits are then queried by looping a call to `vklIterateHit` as long as
the returned lane mask indicates that the iterator is still within the
volume.

``` cpp
int vklIterateHit(VKLHitIterator *iterator, VKLHit *hit);

void vklIterateHit4(const int *valid,
                    VKLHitIterator4 *iterator,
                    VKLHit4 *hit,
                    int *result);

void vklIterateHit8(const int *valid,
                    VKLHitIterator8 *iterator,
                    VKLHit8 *hit,
                    int *result);

void vklIterateHit16(const int *valid,
                     VKLHitIterator16 *iterator,
                     VKLHit16 *hit,
                     int *result);
```

Returned hits consist of the t-value and volume value at that location:

``` cpp
typedef struct
{
  float t;
  float sample;
} VKLHit;

typedef struct
{
  float t[4];
  float sample[4];
} VKLHit4;

typedef struct
{
  float t[8];
  float sample[8];
} VKLHit8;

typedef struct
{
  float t[16];
  float sample[16];
} VKLHit16;
```

For both interval and hit iterators, only the vector-wide API for the
native SIMD width (determined via `vklGetNativeSIMDWidth` can be called.
The scalar versions are always valid. This restriction will likely be
lifted in the future.

# Examples

Open VKL ships with simple tutorial applications demonstrating the basic
usage of the API, as well as full renderers showing recommended usage.

## Tutorials

Simple tutorials can be found in the `examples/` directory. These are:

  - `vklTutorial.c` : usage of the C API
  - `vklTutorial.[cpp,ispc]` : combined usage of the C and ISPC APIs

For quick reference, the contents of `vklTutorial.c` are shown below.

``` cpp

#include <openvkl/openvkl.h>
#include <stdio.h>

void demoScalarAPI(VKLVolume volume)
{
  printf("demo of 1-wide API\n");

  // sample, gradient
  vkl_vec3f coord = {1.f, 1.f, 1.f};
  float sample    = vklComputeSample(volume, &coord);
  vkl_vec3f grad  = vklComputeGradient(volume, &coord);
  printf("\tcoord = %f %f %f\n", coord.x, coord.y, coord.z);
  printf("\t\tsample = %f\n", sample);
  printf("\t\tgrad   = %f %f %f\n\n", grad.x, grad.y, grad.z);

  // value selector setup (note the commit at the end)
  vkl_range1f ranges[2]     = {{10, 20}, {50, 75}};
  int num_ranges            = 2;
  float values[2]           = {32, 96};
  int num_values            = 2;
  VKLValueSelector selector = vklNewValueSelector(volume);
  vklValueSelectorSetRanges(selector, num_ranges, ranges);
  vklValueSelectorSetValues(selector, num_values, values);
  vklCommit(selector);

  // ray definition for iterators
  vkl_vec3f rayOrigin    = {0, 0, 0};
  vkl_vec3f rayDirection = {1, 0, 0};
  vkl_range1f rayTRange  = {0, 200};
  printf("\trayOrigin = %f %f %f\n", rayOrigin.x, rayOrigin.y, rayOrigin.z);
  printf("\trayDirection = %f %f %f\n",
         rayDirection.x,
         rayDirection.y,
         rayDirection.z);
  printf("\trayTRange = %f %f\n", rayTRange.lower, rayTRange.upper);

  // interval iteration
  VKLIntervalIterator intervalIterator;
  vklInitIntervalIterator(&intervalIterator,
                          volume,
                          &rayOrigin,
                          &rayDirection,
                          &rayTRange,
                          selector);

  printf("\n\tinterval iterator for value ranges {%f %f} {%f %f}\n",
         ranges[0].lower,
         ranges[0].upper,
         ranges[1].lower,
         ranges[1].upper);

  for (;;) {
    VKLInterval interval;
    int result = vklIterateInterval(&intervalIterator, &interval);
    if (!result)
      break;
    printf(
        "\t\ttRange (%f %f)\n\t\tvalueRange (%f %f)\n\t\tnominalDeltaT %f\n\n",
        interval.tRange.lower,
        interval.tRange.upper,
        interval.valueRange.lower,
        interval.valueRange.upper,
        interval.nominalDeltaT);
  }

  // hit iteration
  VKLHitIterator hitIterator;
  vklInitHitIterator(
      &hitIterator, volume, &rayOrigin, &rayDirection, &rayTRange, selector);

  printf("\thit iterator for values %f %f\n", values[0], values[1]);

  for (;;) {
    VKLHit hit;
    int result = vklIterateHit(&hitIterator, &hit);
    if (!result)
      break;
    printf("\t\tt %f\n\t\tsample %f\n\n", hit.t, hit.sample);
  }
}

void demoVectorAPI(VKLVolume volume)
{
  printf("demo of 4-wide API (8- and 16- follow the same pattern)\n");

  vkl_vvec3f4 coord4;  // structure-of-array layout
  int valid[4];
  for (int i = 0; i < 4; i++) {
    coord4.x[i] = coord4.y[i] = coord4.z[i] = i;
    valid[i] = -1;  // valid mask: 0 = not valid, -1 = valid
  }

  float sample4[4];
  vkl_vvec3f4 grad4;
  vklComputeSample4(valid, volume, &coord4, sample4);
  vklComputeGradient4(valid, volume, &coord4, &grad4);

  for (int i = 0; i < 4; i++) {
    printf("\tcoord[%d] = %f %f %f\n", i, coord4.x[i], coord4.y[i], coord4.z[i]);
    printf("\t\tsample[%d] = %f\n", i, sample4[i]);
    printf("\t\tgrad[%d]   = %f %f %f\n", i, grad4.x[i], grad4.y[i], grad4.z[i]);
  }
}

int main()
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  int dimensions[] = {128, 128, 128};

  const int numVoxels = dimensions[0] * dimensions[1] * dimensions[2];

  VKLVolume volume = vklNewVolume("structured_regular");
  vklSetVec3i(volume, "dimensions", dimensions[0], dimensions[1], dimensions[2]);
  vklSetVec3f(volume, "gridOrigin", 0, 0, 0);
  vklSetVec3f(volume, "gridSpacing", 1, 1, 1);

  float *voxels = malloc(numVoxels * sizeof(float));

  if (!voxels) {
    printf("failed to allocate voxel memory!\n");
    return 1;
  }

  // x-grad sample volume
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)i;

  VKLData voxelData = vklNewData(numVoxels, VKL_FLOAT, voxels, 0);
  vklSetData(volume, "voxelData", voxelData);
  vklRelease(voxelData);

  vklCommit(volume);

  demoScalarAPI(volume);
  demoVectorAPI(volume);

  vklShutdown();

  free(voxels);

  return 0;
}
```

## Interactive examples

Open VKL also ships with an interactive example application,
`vklExamples`. This interactive viewer demonstrates multiple example
renderers including a path tracer, isosurface renderer (using hit
iterators), and ray marcher. The viewer UI supports switching between
renderers interactively.

Each renderer has both a C++ and ISPC implementation showing recommended
API usage. These implementations are available in the
`examples/interactive/renderers/` directory.

# Building Open VKL from source

The latest Open VKL sources are always available at the [Open VKL GitHub
repository](http://github.com/OpenVKL/openvkl). The default `master`
branch should always point to the latest tested bugfix release.

## Prerequisites

Open VKL currently supports Linux, Mac OS X, and Windows. In addition,
before you can build Open VKL you need the following prerequisites:

  - You can clone the latest Open VKL sources via:
    
        git clone https://github.com/openvkl/openvkl.git

  - To build Open VKL you need [CMake](http://www.cmake.org), any form
    of C++11 compiler (we recommend using GCC, but also support Clang
    and MSVC), and standard Linux development tools. To build the
    examples, you should also have some version of OpenGL.

  - Additionally you require a copy of the [Intel® SPMD Program Compiler
    (ISPC)](http://ispc.github.io), version 1.12.0 or later. Please
    obtain a release of ISPC from the [ISPC downloads
    page](https://ispc.github.io/downloads.html).

  - Open VKL depends on the OSPRay common library, ospcommon. ospcommon
    is available at the [ospcommon GitHub
    repository](https://github.com/ospray/ospcommon).

Depending on your Linux distribution you can install these dependencies
using `yum` or `apt-get`. Some of these packages might already be
installed or might have slightly different names.

## CMake Superbuild

For convenience, Open VKL provides a CMake Superbuild script which will
pull down Open VKL’s dependencies and build Open VKL itself. The result
is an install directory, with each dependency in its own directory.

Run with:

``` bash
mkdir build
cd build
cmake [<VKL_ROOT>/superbuild]
cmake --build .
```

The resulting `install` directory (or the one set with
`CMAKE_INSTALL_PREFIX`) will have everything in it, with one
subdirectory per dependency.

CMake options to note (all have sensible defaults):

  - `CMAKE_INSTALL_PREFIX` will be the root directory where everything
    gets installed.
  - `BUILD_JOBS` sets the number given to `make -j` for parallel builds.
  - `INSTALL_IN_SEPARATE_DIRECTORIES` toggles installation of all
    libraries in separate or the same directory.

For the full set of options, run `ccmake [<VKL_ROOT>/superbuild]`.

## Standard CMake build

Assuming the above prerequisites are all fulfilled, building Open VKL
through CMake is easy:

  - Create a build directory, and go into it
    
        mkdir openvkl/build
        cd openvkl/build
    
    (We do recommend having separate build directories for different
    configurations such as release, debug, etc.).

  - The compiler CMake will use will default to whatever the `CC` and
    `CXX` environment variables point to. Should you want to specify a
    different compiler, run cmake manually while specifying the desired
    compiler. The default compiler on most linux machines is `gcc`, but
    it can be pointed to `clang` instead by executing the following:
    
        cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ..
    
    CMake will now use Clang instead of GCC. If you are ok with using
    the default compiler on your system, then simply skip this step.
    Note that the compiler variables cannot be changed after the first
    `cmake` or `ccmake` run.

  - Open the CMake configuration dialog
    
        ccmake ..

  - Make sure to properly set build mode and enable the components you
    need, etc.; then type ’c’onfigure and ’g’enerate. When back on the
    command prompt, build it using
    
        make

  - You should now have `libopenvkl.so` as well as the tutorial /
    example applications.

# Projects that make use of Open VKL

This page gives a brief (and incomplete) list of other projects that
make use of Open VKL, as well as a set of related links to other
projects and related information.

If you have a project that makes use of Open VKL and would like this to
be listed here, please let us know.

  - [Intel® OSPRay](http://www.ospray.org), a ray tracing based
    rendering engine for high-fidelity visualization

# Projects that are closely related to Open VKL

  - The [Intel® oneAPI Rendering
    Toolkit](https://software.intel.com/en-us/rendering-framework)

  - The [Intel® Embree](http://embree.github.io) Ray Tracing Kernel
    Framework
