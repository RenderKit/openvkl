Intel® Open VKL API
===================

To access the Open VKL API you first need to include the Open VKL header. For
C99 or C++:

    #include <openvkl/openvkl.h>

For the Intel SPMD Program Compiler (ISPC):

    #include <openvkl/openvkl.isph>

This documentation will discuss the C99/C++ API.  The ISPC version has the same
functionality and flavor.  Looking at the headers, the `vklTutorialISPC`
example, and this documentation should be enough to figure it out.

Initialization and shutdown
---------------------------

To use the API, one of the implemented backends must be loaded.  Currently the
only one that exists is the ISPC driver.  ISPC in the name here just refers to
the implementation language -- it can also be used from the C99/C++ APIs.  To
load the module that implements the ISPC driver:

    vklLoadModule("ispc_driver");

The driver then needs to be instantiated:

    VKLDriver driver = vklNewDriver("ispc");

Once a driver is created, you can call

    void vklDriverSetInt(VKLDriver, const char *name, int val);
    void vklDriverSetString(VKLDriver, const char *name, const char *val);

to set parameters on the driver. The following parameters are understood by all
drivers:

  ------ -------------- --------------------------------------------------------
  Type   Name           Description
  ------ -------------- --------------------------------------------------------
  int    logLevel       logging level; valid values are `VKL_LOG_DEBUG`,
                        `VKL_LOG_INFO`, `VKL_LOG_WARNING` and `VKL_LOG_ERROR`

  string logOutput      convenience for setting where log messages go; valid
                        values are `cout`, `cerr` and `none`

  string errorOutput    convenience for setting where error messages go; valid
                        values are `cout`, `cerr` and `none`

  int    numThreads     number of threads which Open VKL can use

  int    flushDenormals sets the `Flush to Zero` and `Denormals are Zero` mode
                        of the MXCSR control and status register (default: 1);
                        see Performance Recommendations section for details
  ------ -------------- --------------------------------------------------------
  : Parameters shared by all drivers.

Once parameters are set, the driver must be committed with

    vklCommitDriver(driver);

Finally, to use the newly committed driver, you must call

    vklSetCurrentDriver(driver);

Users can change parameters on a driver after initialization. In this case the
driver would need to be re-committed. If changes are made to the driver that is
already set as the current driver, it does not need to be set as current again.
The currently set driver can be retrieved at any time by calling

    VKLDriver driver = vklGetCurrentDriver();

Open VKL provides vector-wide versions for several APIs. To determine the native
vector width for the given driver, call:

    int width = vklGetNativeSIMDWidth();

When the application is finished with Open VKL or shutting down, call the
shutdown function:

    vklShutdown();

### Environment variables

The generic driver parameters can be overridden via environment variables for
easy changes to Open VKL’s behavior without needing to change the application
(variables are prefixed by convention with "`OPENVKL_`"):

  ----------------------- ------------------------------------------------------
  Variable                Description
  ----------------------- ------------------------------------------------------
  OPENVKL_LOG_LEVEL       logging level; valid values are `debug`, `info`,
                          `warning` and `error`

  OPENVKL_LOG_OUTPUT      convenience for setting where log messages go; valid
                          values are `cout`, `cerr` and `none`

  OPENVKL_ERROR_OUTPUT    convenience for setting where error messages go; valid
                          values are `cout`, `cerr` and `none`

  OPENVKL_THREADS         number of threads which Open VKL can use

  OPENVKL_FLUSH_DENORMALS sets the `Flush to Zero` and `Denormals are Zero` mode
                          of the MXCSR control and status register (default: 1);
                          see Performance Recommendations section for details
  ----------------------- ------------------------------------------------------
  : Environment variables understood by all drivers.

Note that these environment variables take precedence over values set through
the `vklDriverSet*()` functions.

### Error handling and log messages

The following errors are currently used by Open VKL:

  Name                   Description
  ---------------------- -------------------------------------------------------
  VKL_NO_ERROR           no error occurred
  VKL_UNKNOWN_ERROR      an unknown error occurred
  VKL_INVALID_ARGUMENT   an invalid argument was specified
  VKL_INVALID_OPERATION  the operation is not allowed for the specified object
  VKL_OUT_OF_MEMORY      there is not enough memory to execute the command
  VKL_UNSUPPORTED_CPU    the CPU is not supported (minimum ISA is SSE4.1)
  ---------------------- -------------------------------------------------------
  : Possible error codes, i.e., valid named constants of type `VKLError`.

These error codes are either directly returned by some API functions, or are
recorded to be later queried by the application via

    VKLError vklDriverGetLastErrorCode(VKLDriver);

A more descriptive error message can be queried by calling

    const char* vklDriverGetLastErrorMsg(VKLDriver);

Alternatively, the application can also register a callback function of type

    typedef void (*VKLErrorFunc)(VKLError, const char* message);

via

    void vklDriverSetErrorFunc(VKLDriver, VKLErrorFunc);

to get notified when errors occur. Applications may be interested in messages
which Open VKL emits, whether for debugging or logging events. Applications can
register a callback function of type

    typedef void (*VKLLogFunc)(const char* message);

via

    void vklDriverSetLogFunc(VKLDriver, VKLLogFunc);

which Open VKL will use to emit log messages. Applications can clear either
callback by passing `nullptr` instead of an actual function pointer. By default,
Open VKL uses `cout` and `cerr` to emit log and error messages, respectively.
Note that in addition to setting the above callbacks, this behavior can be
changed via the driver parameters and environment variables described
previously.

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
`vklNew...` API entry point. For example, `vklNewData` and `vklNewVolume`.

In general, modifiable parameters to objects are modified using `vklSet...`
functions based on the type of the parameter being set. The parameter name is
passed as a string. Below are all variants of `vklSet...`.

    void vklSetBool(VKLObject object, const char *name, int b);
    void vklSetFloat(VKLObject object, const char *name, float x);
    void vklSetVec3f(VKLObject object, const char *name, float x, float y, float z);
    void vklSetInt(VKLObject object, const char *name, int x);
    void vklSetVec3i(VKLObject object, const char *name, int x, int y, int z);
    void vklSetData(VKLObject object, const char *name, VKLData data);
    void vklSetString(VKLObject object, const char *name, const char *s);
    void vklSetVoidPtr(VKLObject object, const char *name, void *v);

The exception to this rule is the `VKLValueSelector` object (described in the
iterators section below), which has object-specific set methods.  The reason for
this is to align the C99/C++ API with the ISPC API, which can't use a parameter
method due to language limitations.

After parameters have been set, `vklCommit` must be called on the object to make
them take effect.

Open VKL uses reference counting to manage the lifetime of all objects.
Therefore one cannot explicitly "delete" any object.  Instead, one can indicate
the application does not need or will not access the given object anymore by
calling

    void vklRelease(VKLObject);

This decreases the object's reference count. If the count reaches `0` the
object will automatically be deleted.

Managed data
------------

Large data is passed to Open VKL via a `VKLData` handle created with
`vklNewData`:

    VKLData vklNewData(size_t numItems,
                       VKLDataType dataType,
                       const void *source,
                       VKLDataCreationFlags dataCreationFlags);

Types accepted are listed in `VKLDataType.h`; basic types (`UCHAR`, `INT`,
`UINT`, `LONG`, `ULONG`) exist as both scalar and chunked formats.  The types
accepted vary per volume at the moment; read the volume section below for
specifics.

Data objects can be created as Open VKL owned (`dataCreationFlags =
VKL_DATA_DEFAULT`), in which the library will make a copy of the data for its
use, or shared (`dataCreationFlags = VKL_DATA_SHARED_BUFFER`), which will try
to use the passed pointer for usage.  The library is allowed to copy data when
a volume is committed.

As with other object types, when data objects are no longer needed they should
be released via `vklRelease`.

Observers
---------

Volumes in Open VKL may provide observers to communicate data back to the
application. Observers may be created with

    VKLObserver vklNewObserver(VKLVolume volume, 
                               const char *type);

The volume passed to `vklNewObserver` must already be committed.  Valid
observer type strings are defined by volume implementations (see section
'Volume types' below).

`vklNewObserver` returns `NULL` on failure.

To access the underlying data, an observer must first be mapped using

    const void * vklMapObserver(VKLObserver observer);

If this fails, the function returns `NULL`. `vklMapObserver` may fail on
observers that are already mapped.
On success, the application may query the underlying type and the number of
elements in the buffer using

    VKLDataType vklGetObserverElementType(VKLObserver observer);
    size_t vklGetObserverNumElements(VKLObserver observer);

On failure, these functions return `VKL_UNKNOWN` and `0`, respectively.
Possible data types are defined by the volume that provides the observer , as
are the semantics of the observation. See section 'Volume types' for details.

The pointer returned by `vklMapObserver` may be cast to the type corresponding
to the value returned by `vklGetObserverElementType` to access the observation. 
For example, if `vklGetObserverElementType` returns `VKL_FLOAT`, then
the pointer returned by `vklMapObserver` may be cast to `const float *` to access
up to `vklGetObserverNumElements` consecutive values of type `float`.

Once the application has finished processing the observation, it should unmap
the observer using

    void vklUnmapObserver(VKLObserver observer);

so that the observer may be mapped again.

When an observer is no longer needed, it should be released using `vklRelease`.

The observer API is not thread safe, and these functions should not
be called concurrently on the same object.


Volume types
------------

Open VKL currently supports structured volumes on regular and spherical grids;
unstructured volumes with tetrahedral, wedge, pyramid, and hexaderal primitive
types; and adaptive mesh refinement (AMR) volumes.  These volumes are created
with `vlkNewVolume` with the appropriate type string.

In addition to the usual `vklSet...()` and `vklCommit()` APIs, the volume
bounding box can be queried:

    vkl_box3f vklGetBoundingBox(VKLVolume volume);

The value range of the volume can also be queried:

    vkl_range1f vklGetValueRange(VKLVolume volume);

### Structured Volumes

Structured volumes only need to store the values of the samples, because their
addresses in memory can be easily computed from a 3D position. The dimensions
for all structured volume types are in units of vertices, not cells. For
example, a volume with dimensions $(x, y, z)$ will have $(x-1, y-1, z-1)$ cells
in each dimension. Voxel data provided is assumed vertex-centered, so $x*y*z$
values must be provided.

#### Structured Regular Volumes

A common type of structured volumes are regular grids, which are
created by passing a type string of `"structuredRegular"` to `vklNewVolume`.
The parameters understood by structured regular volumes are summarized in the
table below.

  ------ ----------- -------------  -----------------------------------
  Type   Name            Default    Description
  ------ ----------- -------------  -----------------------------------
  vec3i  dimensions                 number of voxels in each
                                    dimension $(x, y, z)$

  data   data                       VKLData object of voxel data,
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
  : Configuration parameters for structured regular (`"structuredRegular"`) volumes.

#### Structured Spherical Volumes

Structured spherical volumes are also supported, which are created by passing a
type string of `"structuredSpherical"` to `vklNewVolume`. The grid dimensions
and parameters are defined in terms of radial distance ($r$), inclination angle
($\theta$), and azimuthal angle ($\phi$), conforming with the ISO convention for
spherical coordinate systems. The coordinate system and parameters understood by
structured spherical volumes are summarized below.

![Structured spherical volume coordinate system: radial distance ($r$), inclination angle ($\theta$), and azimuthal angle ($\phi$).][imgStructuredSphericalCoords]

  ------ ----------- -------------  -----------------------------------
  Type   Name            Default    Description
  ------ ----------- -------------  -----------------------------------
  vec3i  dimensions                 number of voxels in each
                                    dimension $(r, \theta, \phi)$

  data   data                       VKLData object of voxel data,
                                    supported types are:

                                    `VKL_UCHAR`

                                    `VKL_SHORT`

                                    `VKL_USHORT`

                                    `VKL_FLOAT`

                                    `VKL_DOUBLE`

  vec3f  gridOrigin  $(0, 0, 0)$    origin of the grid in units of
                                    $(r, \theta, \phi)$; angles in degrees

  vec3f  gridSpacing $(1, 1, 1)$    size of the grid cells in units of
                                    $(r, \theta, \phi)$; angles in degrees
  ------ ----------- -------------  -----------------------------------
  : Configuration parameters for structured spherical (`"structuredSpherical"`) volumes.

These grid parameters support flexible specification of spheres, hemispheres,
spherical shells, spherical wedges, and so forth. The grid extents (computed as
$[gridOrigin, gridOrigin + (dimensions - 1) * gridSpacing]$) however must be
constrained such that:

  * $r \geq 0$
  * $0 \leq \theta \leq 180$
  * $0 \leq \phi \leq 360$

### Adaptive Mesh Refinement (AMR) Volumes

Open VKL currently supports block-structured (Berger-Colella) AMR volumes.
Volumes are specified as a list of blocks, which exist at levels of refinement
in potentially overlapping regions.  Blocks exist in a tree structure, with
coarser refinement level blocks containing finer blocks.  The cell width is
equal for all blocks at the same refinement level, though blocks at a coarser
level have a larger cell width than finer levels.

There can be any number of refinement levels and any number of blocks at any
level of refinement. An AMR volume type is created by passing the type string
`"amr"` to `vklNewVolume`.

Blocks are defined by three parameters: their bounds, the refinement level in
which they reside, and the scalar data contained within each block.

Note that cell widths are defined _per refinement level_, not per block.

  -------------- --------------- -----------------  -----------------------------------
  Type           Name                      Default  Description
  -------------- --------------- -----------------  -----------------------------------
  `VKLAMRMethod` method          `VKL_AMR_CURRENT`  `VKLAMRMethod` sampling method.
                                                    Supported methods are:

                                                    `VKL_AMR_CURRENT`

                                                    `VKL_AMR_FINEST`

                                                    `VKL_AMR_OCTANT`

  float[]        cellWidth                    NULL  array of each level's cell width

  box3f[]        block.bounds                 NULL  [data] array of bounds for each AMR
                                                    block

  int[]          block.level                  NULL  array of each block's refinement
                                                    level

  VKLData[]      block.data                   NULL  [data] array of VKLData containing
                                                    the actual scalar voxel data

  vec3f          gridOrigin            $(0, 0, 0)$  origin of the grid in world-space

  vec3f          gridSpacing           $(1, 1, 1)$  size of the grid cells in
                                                    world-space
  -------------- --------------- -----------------  -----------------------------------
  : Configuration parameters for AMR (`"amr"`) volumes.

Lastly, note that the `gridOrigin` and `gridSpacing` parameters act just like
the structured volume equivalent, but they only modify the root (coarsest level)
of refinement.

In particular, Open VKL's AMR implementation was designed to cover
Berger-Colella [1] and Chombo [2] AMR data.  The `method` parameter above
determines the interpolation method used when sampling the volume.

* `VKL_AMR_CURRENT` finds the finest refinement level at that cell and
  interpolates through this "current" level
* `VKL_AMR_FINEST` will interpolate at the closest existing cell in the
  volume-wide finest refinement level regardless of the sample cell's level
* `VKL_AMR_OCTANT` interpolates through all available refinement levels at that
  cell. This method avoids discontinuities at refinement level boundaries at
  the cost of performance

Details and more information can be found in the publication for the
implementation [3].

1. M. J. Berger, and P. Colella. "Local adaptive mesh refinement for
   shock hydrodynamics." Journal of Computational Physics 82.1 (1989): 64-84.
   DOI: 10.1016/0021-9991(89)90035-1
2. M. Adams, P. Colella, D. T. Graves, J.N. Johnson, N.D. Keen, T. J. Ligocki.
   D. F. Martin. P.W. McCorquodale, D. Modiano. P.O. Schwartz, T.D. Sternberg
   and B. Van Straalen, Chombo Software Package for AMR Applications - Design
   Document,  Lawrence Berkeley National Laboratory Technical Report
   LBNL-6616E.
3. I. Wald, C. Brownlee, W. Usher, and A. Knoll. CPU volume rendering of
   adaptive mesh refinement data. SIGGRAPH Asia 2017 Symposium on Visualization
   on - SA ’17, 18(8), 1–8. DOI: 10.1145/3139295.3139305

### Unstructured Volumes

Unstructured volumes can have their topology and geometry freely defined.
Geometry can be composed of tetrahedral, hexahedral, wedge or pyramid cell
types. The data format used is compatible with VTK and consists of multiple
arrays: vertex positions and values, vertex indices, cell start indices, cell
types, and cell values. An unstructured volume type is created by passing the
type string `"unstructured"` to `vklNewVolume`.

Sampled cell values can be specified either per-vertex (`vertex.data`) or
per-cell (`cell.data`). If both arrays are set, `cell.data` takes precedence.

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

To maintain VTK data compatibility an index array may be specified via the
`indexPrefixed` array that allows vertex indices to be interleaved with cell
sizes in the following format: $n, id_1, ..., id_n, m, id_1, ..., id_m$.

  -------------------  ------------------  --------  ---------------------------------------
  Type                 Name                Default   Description
  -------------------  ------------------  --------  ---------------------------------------
  vec3f[]              vertex.position               [data] array of vertex positions

  float[]              vertex.data                   [data] array of vertex data values to
                                                     be sampled

  uint32[] / uint64[]  index                         [data] array of indices (into the
                                                     vertex array(s)) that form cells

  uint32[] / uint64[]  indexPrefixed                 alternative [data] array of indices
                                                     compatible to VTK, where the indices of
                                                     each cell are prefixed with the number
                                                     of vertices

  uint32[] / uint64[]  cell.index                    [data] array of locations (into the
                                                     index array), specifying the first index
                                                     of each cell

  float[]              cell.data                     [data] array of cell data values to be
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

  bool                 precomputedNormals     false  whether to accelerate by precomputing,
                                                     at a cost of 12 bytes/face
  -------------------  ------------------  --------  ---------------------------------------
  : Configuration parameters for unstructured (`"unstructured"`) volumes.

### VDB Volumes

VDB volumes implement a data structure that is very similar to the data structure
outlined in Museth [1].

The data structure is a hierarchical regular grid at its core: Nodes are regular grids,
and each grid cell may either store a constant value (this is called a tile), or
child pointers.

Nodes in VDB trees are wide: Nodes on the first level have a resolution of 32^3 voxels
by default, on the next level 16^3, and on the leaf level 8^3 voxels. All nodes
on a given level have the same resolution. This makes it easy to find the node
containing a coordinate using shift operations (cp. [1]).

VDB leaf nodes are implicit in Open VKL: they are stored as pointers to user-provided data.

![Structure of `"vdb"` volumes in the default configuration][imgVdbStructure]

VDB volumes interpret input data as constant cells (which are then potentially filtered).
This is in contrast to `structuredRegular` volumes, which have a vertex-centered
interpretation.

The VDB implementation in Open VKL follows the following goals:

  - Efficient data structure traversal on vector architectures.

  - Enable the use of industry-standard .vdb files created through the OpenVDB library.

  - Compatibility with OpenVDB on a leaf data level, so that .vdb files may be loaded
    with minimal overhead.

VDB volumes have the following parameters:

  ------------  ----------------  ---------------------- ---------------------------------------
  Type          Name              Default                Description
  ------------  ----------------  ---------------------- ---------------------------------------
  int           type                                     The field type. Only `VKL_FLOAT` is 
                                                         supported at the moment. Use the enum
                                                         `VKLDataType` for named constants.

  int           filter            `VKL_FILTER_TRILINEAR` The filter used for reconstructing the
                                                         field. Use `VKLFilter` for named 
                                                         constants.

  int           maxSamplingDepth  `VKL_VDB_NUM_LEVELS`   Do not descend further than to this
                                                         depth during sampling.

  int           maxIteratorDepth  3                      Do not descend further than to this
                                                         depth during interval iteration.
                                                         The maximum value is 3.
                            
  float[]       indexToObject     1, 0, 0,               An array of 12 values of type `float` 
                                  0, 1, 0,               that define the transformation from
                                  0, 0, 1,               index space to object space.
                                  0, 0, 0                In index space, the grid is an 
                                                         axis-aligned regular grid, and leaf
                                                         voxels have size (1,1,1).
                                                         The first 9 values are interpreted
                                                         as a row-major linear transformation
                                                         matrix. The last 3 values are the
                                                         translation of the grid origin.

  uint32[]      level                                    For each input node, the level on
                                                         which this node exists. Levels are
                                                         counted from the root level (0) down.
                                                         Input nodes may be on levels 
                                                         [1, `VKL_VDB_NUM_LEVELS-1`].

  vec3i[]       origin                                   For each input node, the node origin
                                                         index.

  uint32[]      format                                   For each input node, the data format.
                                                         Currently supported are 
                                                         `VKL_VDB_FORMAT_TILE` for tiles,
                                                         and `VKL_VDB_FORMAT_CONSTANT` for
                                                         nodes that are dense regular grids,
                                                         but temporally constant.

  VKLData[]     data                                     Node data. Nodes with format 
                                                         `VKL_VDB_FORMAT_TILE` are expected to
                                                         have single-entry arrays. Nodes with
                                                         format `VKL_VDB_FORMAT_CONSTANT` are
                                                         expected to have arrays with
                                                         `vklVdbLevelNumVoxels(level[i])` 
                                                         entries.
  ------------  ----------------  ---------------------- ---------------------------------------
  : Configuration parameters for VDB (`"vdb"`) volumes.

The level, origin, format, and data parameters must have the same size, and there must
be at least one valid node or `commit()` will fail.

VDB volumes support the following observers:

  --------------  -----------  -------------------------------------------------------------
  Name            Buffer Type  Description
  --------------  -----------  -------------------------------------------------------------
  LeafNodeAccess  uint32[]     This observer returns an array with as many entries as 
                               input nodes were passed. If the input node i was accessed
                               during traversal, then the ith entry in this array has a
                               nonzero value.
                               This can be used for on-demand loading of leaf nodes.
  --------------  --------------------------------------------------------------------------
  : Observers supported by VDB (`"vdb"`) volumes.


#### Major differences to OpenVDB

  - Open VKL implements sampling in ISPC, and can exploit wide SIMD architectures.

  - VDB volumes in Open VKL are read-only once committed, and designed for rendering only. 
    Authoring or manipulating datasets is not in the scope of this implementation.

  - The only supported field type is `VKL_FLOAT` at this point. Other field types
    may be supported in the future.

  - The root level in Open VKL has a single node with resolution 64^3 (cp. [1]. OpenVDB 
    uses a hash map, instead).

  - The tree topology can be configured at compile time, but this happens through
    the CMake option `VKL_VDB_LOG_RESOLUTION`. By default this is set to "6;5;4;3",
    which means that there are four levels, the root node has a resolution of 
    (2^6^3 = 64^3), first level nodes a resolution of (2^5^3 = 32^3), and so on.

#### Loading OpenVDB .vdb files

Files generated with OpenVDB can be loaded easily since Open VKL `vdb` volumes
implement the same leaf data layout. This means that OpenVDB leaf data pointers
can be passed to Open VKL using shared data buffers, avoiding copy operations.

An example of this can be found in `vdb_util/include/openvkl/OpenVdbGrid.h`,
where the class `OpenVdbFloatGrid` encapsulates the necessary operations. This
class is also accessible through the `vklExamples` application using the
`-file` and `-field` command line arguments. 

To use this example feature, compile Open VKL with `OpenVDB_ROOT` pointing to
the OpenVDB prefix.


1. Museth, K. VDB: High-Resolution Sparse Volumes with Dynamic Topology.
   ACM Transactions on Graphics 32(3), 2013. DOI: 10.1145/2487228.2487235

Sampling
--------

Computing the value of a volume at an object space coordinate is done using the
sampling API.  NaN is returned for probe points outside the volume.

The scalar API just takes a volume and coordinate, and returns a float value.

    float vklComputeSample(VKLVolume volume, const vkl_vec3f *objectCoordinates);

Vector versions allow sampling at 4, 8, or 16 positions at once.  Depending on
the machine type and Open VKL driver implementation, these can give greater
performance.  An active lane mask `valid` is passed in as an array of integers;
set 0 for lanes to be ignored, -1 for active lanes.

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

Intervals can then be processed by calling `vklIterateInterval` as long as the
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

The intervals returned have a t-value range, a value range, and a
`nominalDeltaT` which is approximately the step size that should be used to
walk through the interval, if desired.  The number and length of intervals
returned is volume type implementation dependent.  There is currently no way of
requesting a particular splitting.

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

Querying for particular values is done using a `VKLHitIterator` in much the
same fashion.  This API could be used, for example, to find isosurfaces.
Again, a user allocated `VKLHitIterator` of the desired width must be
initialized:

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

Performance Recommendations
===========================

MXCSR control and status register
---------------------------------

It is strongly recommended to have the `Flush to Zero` and `Denormals are Zero`
mode of the MXCSR control and status register enabled for each thread before
calling the sampling, gradient, or interval API functions. Otherwise, under some
circumstances special handling of denormalized floating point numbers can
significantly reduce application and Open VKL performance. The driver parameter
`flushDenormals` or environment variable `OPENVKL_FLUSH_DENORMALS` can be used
to toggle this mode; by default it is enabled. Alternatively, when using Open
VKL together with the Intel® Threading Building Blocks, it is sufficient to
execute the following code at the beginning of the application main thread
(before the creation of the `tbb::task_scheduler_init` object):

    #include <xmmintrin.h>
    #include <pmmintrin.h>
    ...
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

If using a different tasking system, make sure each thread calling into
Open VKL has the proper mode set.
