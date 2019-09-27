Intel® Open VKL API
===================

To access the Open VKL API you first need to include the Open VKL header. For
C99 or C++:

    #include <openvkl/openvkl.h>

For the Intel SPMD Program Compiler (ISPC):

    #include <openvkl/openvkl.isph>

This documentation will discuss the C99/C++ API.  The ISPC version has the same
functionality and flavor.  Looking at the headers, the vklTutorialISPC example,
and this documentation should be enough to figure it out.

Basic data types
----------------

Open VKL defines 3-component vectors of integer and vector types:

    typedef struct
    {
      int x, y, z;
    } vkl_vec3i;

    typedef struct
    {
      float x, y, z;
    } vkl_vec3f;

Vector versions of these are also defined in structure-of-array format for 4, 8,
and 16 wide types.

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

1-D range and 3-D ranges are defined as ranges and boxes, with no vector
versions:

    typedef struct
    {
      float lower, upper;
    } vkl_range1f;

    typedef struct
    {
      vkl_vec3f lower, upper;
    } vkl_box3f;

Object model
------------

Objects in Open VKL are exposed to the APIs as handles with internal reference
counting for lifetime determination.  Objects are created with particular type's
`vklNew...` API entry point.

In general, modifyable parameters to objects are modified with string
parameterized `vklSet<type>(<object>, <string parameter name>, ...)`.

    void vklSetBool(VKLObject object, const char *name, int b);
    void vklSetFloat(VKLObject object, const char *name, float x);
    void vklSetVec3f(VKLObject object, const char *name, float x, float y, float z);
    void vklSetInt(VKLObject object, const char *name, int x);
    void vklSetVec3i(VKLObject object, const char *name, int x, int y, int z);
    void vklSetData(VKLObject object, const char *name, VKLData data);
    void vklSetString(VKLObject object, const char *name, const char *s);
    void vklSetVoidPtr(VKLObject object, const char *name, void *v);

The exception to this rule is the `VKLValueSelector` object (described in the
iterators section below) which has object-specific set methods.  The reason for
this is to align the C99/C++ API with the ISPC API, which can't use a parameter
method due to language limitations.

After parameters have been set, `vklCommit` must be called on the object to make
them take effect.

Since Open VKL uses reference counting to manage the lifetime of all objects,
one cannot explicitly "delete" any object. Instead, to indicate that the
application does not need and does not access the given object anymore, call

    void vklRelease(VKLObject);

This decreases its reference count and if the count reaches `0` the object will
automatically be deleted.

Managed data
------------

Large data is passed to Open VKL via a VKLData handle created with `vklNewData`:

    VKLData vklNewData(size_t numItems,
                       VKLDataType dataType,
                       const void *source,
                       VKLDataCreationFlags dataCreationFlags);

Types accepted are listed in VKLDataType.h; basic types (UCHAR, INT, UINT, LONG,
ULONG) exist as both scalar and chunked formats.  The types accepted vary per
volume at the moment; read the volume section below for specifics.

Data objects can be created as Open VKL owned (`VKL_DATA_DEFAULT`), in which the
library will make a copy of the data for its use, or shared
(`VKL_DATA_SHARED_BUFFER`), which will try to use the passed pointer for usage.
The library is allowed to copy data when a volume is committed.

As with other object types, when data objects are no longer needed they should
be released via `vklRelease`.

Initialization and shutdown
---------------------------

To use the API, one of the implemented backends must be loaded.  Currently the
only one that exists is the ISPC driver.  ISPC in the name here just refers to
the implementation language -- it can also be used from the C99/C++ APIs.  To
load the module that implements the ISPC driver:

    vklLoadModule("ispc_driver");

The driver then needs to be selected:

    VKLDriver driver = vklNewDriver("ispc");
    vklCommitDriver(driver);
    vklSetCurrentDriver(driver);

Open VKL provides vector-wide versions for several APIs. To determine the native
vector width for the given driver, call:

    int width = vklGetNativeSIMDWidth();

When the application is finished with Open VKL or shutting down, call the
shutdown function:

    vklShutdown();

Volume types
------------

Open VKL currently supports structured regular volumes (with user specified
spacing on axes); unstructured volumes with tetrahedral, wedge, pyramid, and
hexaderal primitive types; and adaptive mesh refinement (AMR) volumes.  These
volumes are created with `vlkNewVolume` with the appropriate type string.

In addition to the usual `vklSet...()` and `vklCommit()` APIs, the volume
bounding box can be queried:

    vkl_box3f vklGetBoundingBox(VKLVolume volume);

### Structured Volume

Structured volumes only need to store the values of the samples, because their
addresses in memory can be easily computed from a 3D position. A common type of
structured volumes are regular grids.  Structured grids are created by passing a
type string of `"structured_regular"` to `vklNewVolume`.

The  parameters understood by structured volumes are summarized in the table
below.

  ------ ----------- -------------  -----------------------------------
  Type   Name            Default    Description
  ------ ----------- -------------  -----------------------------------
  vec3i  dimensions                 number of voxels in each
                                    dimension $(x, y, z)$

  data   voxelData                  VKLData object of voxel data,
                                    supported types are:

                                    `VKL_UCHAR`

                                    `VKL_SHORT`

                                    `VKL_USHORT`

                                    `VKL_FLOAT`

                                    `VKL_DOUBLE`

  vec3f  gridOrigin  $(0, 0, 0)$    origin of the grid in world-space

  vec3f  gridSpacing $(1, 1, 1)$    size of the grid cells in
                                    world-space
  ------ ----------- -------------  -----------------------------------
  : Additional configuration parameters for structured volumes.

The dimensions for structured volumes are in terms units vertices, not cells.
For example, a volume with dimensions $(x, y, z)$ will have $(x-1, y-1, z-1)$
cells in each dimension. Voxel data provided is assumed vertex-centered, so
$x*y*z$ values must be provided.

### Adaptive Mesh Refinement (AMR) Volume

AMR volumes are specified as a list of blocks, which are levels of refinement in
potentially overlapping regions. There can be any number of refinement levels
and any number of blocks at any level of refinement. An AMR volume type is
created by passing the type string `"amr"` to `vklNewVolume`.

Blocks are defined by four parameters: their bounds, the refinement level in
which they reside, their cell width, and the scalar data contained within them.

  -------------- --------------- -----------------  -----------------------------------
  Type           Name                      Default  Description
  -------------- --------------- -----------------  -----------------------------------
  `VKLAMRMethod` method          `VKL_AMR_CURRENT`  `VKLAMRMethod` sampling method.
                                                    Supported methods are:

                                                    `VKL_AMR_CURRENT`

                                                    `VKL_AMR_FINEST`

                                                    `VKL_AMR_OCTANT`

  box3f[]        block.bounds                 NULL  [data] array of bounds for each AMR
                                                    block

  int[]          block.level                  NULL  array of each block's refinement
                                                    level

  float[]        block.cellWidth              NULL  array of each block's cell width

  VKLData[]      block.data                   NULL  [data] array of VKLData containing
                                                    the actual scalar voxel data

  vec3f          gridOrigin            $(0, 0, 0)$  origin of the grid in world-space

  vec3f          gridSpacing           $(1, 1, 1)$  size of the grid cells in
                                                    world-space
  -------------- --------------- -----------------  -----------------------------------
  : Additional configuration parameters for AMR volumes.

Lastly, note that the `gridOrigin` and `gridSpacing` parameters act just like
the structured volume equivalent, but they only modify the root (coarsest level)
of refinement.

### Unstructured Volumes

Unstructured volumes can have their topology and geometry freely defined.
Geometry can be composed of tetrahedral, hexahedral, wedge or pyramid cell
types. The data format used is compatible with VTK and consists of multiple
arrays: vertex positions and values, vertex indices, cell start indices, cell
types, and cell values. An unstructured volume type is created by passing the
type string `"unstructured_volume"` to `vklNewVolume`.

Sampled cell values can be specified either per-vertex (`vertex.value`) or
per-cell (`cell.value`). If both arrays are set, `cell.value` takes precedence.

Similar to a mesh, each cell is formed by a group of indices into the vertices.
For each vertex, the corresponding (by array index) data value will be used for
sampling when rendering, if specified. The index order for a tetrahedron is the
same as `VTK_TETRA`: bottom triangle counterclockwise, then the top vertex.

For hexahedral cells, each hexahedron is formed by a group of eight indices into
the vertices and data values. Vertex ordering is the same as `VTK_HEXAHEDRON`:
four bottom vertices counterclockwise, then top four counterclockwise.

For wedge cells, each wedge is formed by a group of six indices into the
vertices and data values. Vertex ordering is the same as `VTK_WEDGE`: three
bottom vertices counterclockwise, then top three counterclockwise.

For pyramid cells, each cell is formed by a group of five indices into the
vertices and data values. Vertex ordering is the same as `VTK_PYRAMID`: four
bottom vertices counterclockwise, then the top vertex.

To maintain VTK data compatibility an index array may be specified via
`indexPrefixed` array that allow vertex indices to be interleaved with cell
sizes in the following format: $n, id_1, ..., id_n, m, id_1, ..., id_m$.

  -------------------  ------------------  --------  ---------------------------------------
  Type                 Name                Default   Description
  -------------------  ------------------  --------  ---------------------------------------
  vec3f[]              vertex.position               [data] array of vertex positions

  float[]              vertex.value                  [data] array of vertex data values to
                                                     be sampled

  uint32[] / uint64[]  index                         [data] array of indices (into the
                                                     vertex array(s)) that form cells

  uint32[] / uint64[]  indexPrefixed                 alternative [data] array of indices
                                                     compatible to VTK, where the indices of
                                                     each cell are prefixed with the number
                                                     of vertices

  uint32[] / uint64[]  cell                          [data] array of locations (into the
                                                     index array), specifying the first index
                                                     of each cell

  float[]              cell.value                    [data] array of cell data values to be
                                                     sampled

  uint8[]              cell.type                     [data] array of cell types
                                                     (VTK compatible). Supported types are:

                                                     `VKL_TETRAHEDRON`

                                                     `VKL_HEXAHEDRON`

                                                     `VKL_WEDGE`

                                                     `VKL_PYRAMID`

  bool                 hexIterative           false  hexahedron
                                                     interpolation method, defaults to fast
                                                     non-iterative version which could have
                                                     rendering inaccuracies may appear
                                                     if hex is not parallelepiped

  bool                 precomputedNormals      true  whether to accelerate by precomputing,
                                                     at a cost of 12 bytes/face
  -------------------  ------------------  --------  ---------------------------------------
  : Additional configuration parameters for unstructured volumes.


Sampling
--------

Computing the value of a volume at an object space coordinate is done using the
sampling API.  NaN is returned for probe point(s) outside the volume.

The scalar API just takes a volume and coordinate, and returns a float value.

    float vklComputeSample(VKLVolume volume, const vkl_vec3f *objectCoordinates);

Vector versions allow sampling at 4, 8, or 16 positions at once.  Depending on
the machine type and Open VKL driver implementation, these can give greater
performance.  An active lane mask is passed in as an array of integers; set 0
for lanes to be ignored, -1 for active lanes.

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

All of the above sampling APIs can be used, regardless of the driver's native
SIMD width.

Gradients
---------

In a very similar API to `vlkComputeSample`, `vlkComputeGradient` queries the
value gradient at an object space coordinate.  Again, a scalar API, now
returning a vec3f instead of a float. NaN values are returned for points outside
the volume.

    vkl_vec3f vklComputeGradient(VKLVolume volume,
                                 const vkl_vec3f *objectCoordinates);

Vector versions are also provided:

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

All of the above gradient APIs can be used, regardless of the driver's native
SIMD width.

Iterators
---------

Open VKL has APIs to search for particular volume values along a ray.  Queries
can be for ranges of volume values (`vklIterateInterval`) or for particular
values (`vklIterateHit`).  The desired values are set in a `VKLValueSelector`,
which needs to be created, filled in with values, and then committed.

    VKLValueSelector vklNewValueSelector(VKLVolume volume);

    void vklValueSelectorSetRanges(VKLValueSelector valueSelector,
                                   size_t numRanges,
                                   const vkl_range1f *ranges);

    void vklValueSelectorSetValues(VKLValueSelector valueSelector,
                                   size_t numValues,
                                   const float *values);

To query an interval, a `VKLIntervalIterator` of scalar or vector width must be
initialized with `vklInitIntervalIterator`.  The iterator structure is allocated
and belongs to the caller, and initialized by the following functions.

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

Intervals can then be procesed by calling `vklIterateInterval` as long as the
returned lane masks indicates that the iterator is still within the volume:

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

The intervals returned have a t-value range, a value range, and a nominalDeltaT
which is approximately the step size that should be used to walk through the
interval, if desired.  The number and length of intervals returned is volume
type implementation dependent.  There is currently no way of requesting a
particular splitting.

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

Querying for particular values are done using a `VKLHitIterator` in much the
same fashion.  This API could be used, for example, to find isosurfaces.  Again,
a user allocated `VKLHitIterator` of the desired width must be initialized:

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

Hits are then queried by looping a call to `vklIterateHit` as long as the
returned lane mask indicates that the iterator is still within the volume.

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

Returned hits consist of the t-value and volume value at that location:

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

For both interval and hit iterators, only the vector-wide API for the native
SIMD width (determined via `vklGetNativeSIMDWidth` can be called. The scalar
versions are always valid. This restriction will likely be lifted in the future.
