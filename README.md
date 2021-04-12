# Intel® Open Volume Kernel Library

This is release v0.13.0 of Intel® Open VKL. For changes and new features
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

### Open VKL 0.12.1

  - Fixed bug in VDB volume interval iterator implementation which could
    lead to missed intervals or incorrect value ranges in returned
    intervals

### Open VKL 0.12.0

  - Added support for temporally varying volumes with associated API
    changes for sampling, gradients, and hit iteration. This feature can
    be used to enable motion blurred rendering
  - Structured regular volumes:
      - Support for temporally structured and temporally unstructured
        (TUV) input data
      - Improved `nominalDeltaT` for interval iteration
      - Interval iterator robustness improvements for axis-aligned rays
      - Sampling performance improvements
  - VDB volumes:
      - Multi-attribute support (including three-component float grids)
      - Interval iterator robustness improvements for axis-aligned rays
      - Performance improvements for scalar sampling
      - Now restricting volumes to exactly four levels
      - Allowing leaf nodes on the lowest level only
  - Unstructured volumes:
      - Improved `nominalDeltaT` for interval iteration
  - `vdb_util` updates:
      - Support for loading multi-attribute .vdb files (`float` and
        `Vec3s` grids)
      - Fix order of rotation matrix coefficients loaded from .vdb files
  - vklExamples additions demonstrating:
      - Motion blurred rendering on temporally structured and temporally
        unstructured volumes (`structuredRegular` only)
      - Support for `vdb` multi-attribute volumes
      - Hit iterator time support
  - Superbuild updates to:
      - Embree 3.12.2
      - rkcommon 1.6.0
      - ISPC 1.15.0
      - OpenVDB 8.0.0
  - Minimum rkcommon version is now 1.6.0

### Open VKL 0.11.0

  - Introduced API support for multi-attribute volumes, including APIs
    for sampling multiple attributes simultaneously
      - Initially only `structuredRegular` and `structuredSpherical`
        volume types support multi-attribute data
  - Iterator APIs now work on sampler objects rather than volumes,
    supporting finer-grained configurability
  - Observers can now be created for both volume and sampler objects
      - `LeafNodeAccess` observers must now be created on sampler
        objects
  - Log and error callbacks now support a user pointer
  - `vdb` volume interval iterators:
      - Added support for elementary cell iteration when
        `maxIteratorDepth` is set to `VKL_VDB_NUM_LEVELS`-1
      - Up to 2x faster iteration
  - `unstructured` and `particle` volume interval iterators:
      - Improved interior empty space skipping behavior
      - Added support for configurable iterator depth via the
        `maxIteratorDepth` parameter
  - Added support for filter modes in `structuredRegular` and
    `structuredSpherical` volumes
  - `amr` volumes now support `method` parameter on sampler objects
  - Added new `interval_iterator_debug` renderer in `vklExamples` to
    visualize interval iteration behavior
  - Hit iterator accuracy improvements for `unstructured` volumes
  - Fixed bugs in `amr` and `vdb` volume bounding box computations
  - Fixed bug in `unstructured` volume gradient computations near empty
    regions
  - Minimum ISPC version is now v1.14.1

### Open VKL 0.10.0 (alpha)

  - Added new `particle` volume type supporting Gaussian radial basis
    functions
  - Introduced `VKLSampler` objects allowing configuration of sampling
    and gradient behavior
  - Added stream-wide sampling and gradient APIs
  - Introduced a new way to allocate iterators, giving the user more
    freedom in choosing allocation schemes and reducing iterator size
  - Added support for strided data arrays
  - Added gradient implementations for `amr` and `vdb` volumes
  - Hit iterator accuracy improvements for `amr`, `structuredSpherical`,
    `unstructured`, and `vdb` volumes
  - Up to 4x performance improvement for `structuredRegular` and
    `structuredSpherical` sampling for volumes in the 1-2GB range
  - Up to 2x performance improvement for `structuredRegular` interval
    iteration
  - Improved commit speed for `unstructured` volumes
  - Improved value range computation in `vdb` volumes
  - Improved isosurface shading in `vklExamples`
  - Improved parameter validation across all volume types
  - Aligned `VKLHit[4,8,16]` and `VKLInterval[4,8,16]` structs
  - Added hit epsilon to `VKLHit[4,8,16]`
  - Updated parameter names for `vdb` volumes
  - Renamed `VKLVdbLeafFormat` to `VKLFormat`
  - Fixed incorrect use of system-installed CMake in superbuild while
    building dependencies
  - Fixed various memory leaks
  - Fixed crashes which could occur in `VdbVolume::cleanup()` and
    `vklShutdown()`
  - Moved from ospcommon to rkcommon v1.4.1

### Open VKL 0.9.0 (alpha)

  - Added support for VDB sparse structured volumes (`"vdb"` volume
    type)
  - Added `vdb_util` library to simplify instantiation of VDB volumes,
    and support loading of .vdb files using OpenVDB
  - Added `VKLObserver` and associated APIs, which may used by volume
    types to pass information back to the application
      - A `LeafNodeAccess` observer is provided for VDB volumes to
        support on-demand loading of leaf nodes
  - Structured regular volumes:
      - Up to 6x performance improvement for scalar iterator
        initialization
      - Up to 2x performance improvement for scalar iterator iteration
  - General improvements to the CMake Superbuild for building Open VKL
    and all associated dependencies
  - Allowing instantiation of ISPC driver for any supported SIMD width
    (in addition to the default automatically selected width)
  - Volume type names are now camelCase (legacy snake\_case type names
    are deprecated), impacting `structuredRegular` and
    `structuredSpherical` volumes
  - Enabling `flushDenormals` driver mode by default
  - Aligning public `vkl_vvec3f[4,8,16]` and `vkl_vrange1f[4,8,16]`
    types
  - Added `VKL_LOG_NONE` log level
  - Fixed bug in `vklExamples` which could lead to improper rendering on
    macOS Catalina
  - Fixed bug in unstructured volume interval iterator which could lead
    to errors with some combinations of lane masks
  - Now providing binary releases for Linux, macOS, and Windows

### Open VKL 0.8.0 (alpha)

  - Added support for structured volumes on spherical grids
    (`"structured_spherical"` volume type)
  - Structured regular volumes:
      - Up to 8x performance improvement for scalar (single-wide)
        sampling
      - Fixed hit iterator bug which could lead to isosurfacing
        artifacts
      - Renamed `voxelData` parameter to `data`
  - Unstructured volumes:
      - Up to 4x performance improvement for scalar (single-wide)
        sampling
      - Improved interval iterator implementation for more efficient
        space skipping and tighter value bounds on returned intervals
      - Now using Embree for BVH builds for faster build times / volume
        commits
      - Renamed `vertex.value` and `cell.value` parameters to
        `vertex.data` and `cell.data`, respectively
  - AMR volumes:
      - renamed `block.cellWidth` parameter to `cellWidth`, and
        clarified API documentation
  - Added `vklGetValueRange()` API for querying volume value ranges
  - Added new driver parameters, APIs, and environment variables
    allowing user control of log levels, log / error output redirection,
    number of threads, and other options
  - `vklIterateHit[4,8,16]()` and `vklIterateInterval[4,8,16]()` calls
    now only populate hit / interval data for active lanes
  - Changed `VKLDataType` enum values for better forward compatibility
  - ISPC-side hit and interval iterator objects must now be declared
    `varying`
  - More flexible ISA build configuration through `OPENVKL_MAX_ISA` and
    `OPENVKL_ISA_*` CMake build options
  - Minimum ospcommon version is now 1.1.0

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
happen to have a fix for it, you can also send us a pull request); you
may also contact us via email at <openvkl@googlegroups.com>.

Join our [mailing
list](https://groups.google.com/forum/#!forum/openvkl-announce/join) to
receive release announcements and major news regarding Open VKL.

# Open VKL API

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
Currently the only one that exists is the CPU device. To load the module
that implements the CPU device:

``` cpp
vklLoadModule("cpu_device");
```

The device then needs to be instantiated:

``` cpp
VKLDevice device = vklNewDevice("cpu");
```

By default, the CPU device selects the maximum supported SIMD width (and
associated ISA) for the system. Optionally, a specific width may be
requested using the `cpu_4`, `cpu_8`, or `cpu_16` device names. Note
that the system must support the given width (SSE4.1 for 4-wide, AVX for
8-wide, and AVX512 for 16-wide).

Once a device is created, you can call

``` cpp
void vklDeviceSetInt(VKLDevice, const char *name, int val);
void vklDeviceSetString(VKLDevice, const char *name, const char *val);
```

to set parameters on the device. The following parameters are understood
by all devices:

| Type   | Name           | Description                                                                                                                                                       |
| :----- | :------------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| int    | logLevel       | logging level; valid values are `VKL_LOG_DEBUG`, `VKL_LOG_INFO`, `VKL_LOG_WARNING`, `VKL_LOG_ERROR` and `VKL_LOG_NONE`                                            |
| string | logOutput      | convenience for setting where log messages go; valid values are `cout`, `cerr` and `none`                                                                         |
| string | errorOutput    | convenience for setting where error messages go; valid values are `cout`, `cerr` and `none`                                                                       |
| int    | numThreads     | number of threads which Open VKL can use                                                                                                                          |
| int    | flushDenormals | sets the `Flush to Zero` and `Denormals are Zero` mode of the MXCSR control and status register (default: 1); see Performance Recommendations section for details |

Parameters shared by all devices.

Once parameters are set, the device must be committed with

``` cpp
vklCommitDevice(device);
```

Finally, to use the newly committed device, you must call

``` cpp
vklSetCurrentDevice(device);
```

Users can change parameters on a device after initialization. In this
case the device would need to be re-committed. If changes are made to
the device that is already set as the current device, it does not need
to be set as current again. The currently set device can be retrieved at
any time by calling

``` cpp
VKLDevice device = vklGetCurrentDevice();
```

Open VKL provides vector-wide versions for several APIs. To determine
the native vector width for the given device, call:

``` cpp
int width = vklGetNativeSIMDWidth();
```

When the application is finished with Open VKL or shutting down, call
the shutdown function:

``` cpp
vklShutdown();
```

### Environment variables

The generic device parameters can be overridden via environment
variables for easy changes to Open VKL’s behavior without needing to
change the application (variables are prefixed by convention with
“`OPENVKL_`”):

| Variable                  | Description                                                                                                                                                       |
| :------------------------ | :---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| OPENVKL\_LOG\_LEVEL       | logging level; valid values are `debug`, `info`, `warning`, `error` and `none`                                                                                    |
| OPENVKL\_LOG\_OUTPUT      | convenience for setting where log messages go; valid values are `cout`, `cerr` and `none`                                                                         |
| OPENVKL\_ERROR\_OUTPUT    | convenience for setting where error messages go; valid values are `cout`, `cerr` and `none`                                                                       |
| OPENVKL\_THREADS          | number of threads which Open VKL can use                                                                                                                          |
| OPENVKL\_FLUSH\_DENORMALS | sets the `Flush to Zero` and `Denormals are Zero` mode of the MXCSR control and status register (default: 1); see Performance Recommendations section for details |

Environment variables understood by all devices.

Note that these environment variables take precedence over values set
through the `vklDeviceSet*()` functions.

Additionally, the CPU device’s default SIMD width can be overriden at
run time with the `OPENVKL_CPU_DEVICE_DEFAULT_WIDTH` environment
variable. Legal values are 4, 8, or 16. This setting is only applicable
when the generic `cpu` device is instantiated; if a specific width is
requested via the `cpu_[4,8,16]` device names then the environment
variable is ignored.

### Error handling and log messages

The following errors are currently used by Open VKL:

| Name                    | Description                                           |
| :---------------------- | :---------------------------------------------------- |
| VKL\_NO\_ERROR          | no error occurred                                     |
| VKL\_UNKNOWN\_ERROR     | an unknown error occurred                             |
| VKL\_INVALID\_ARGUMENT  | an invalid argument was specified                     |
| VKL\_INVALID\_OPERATION | the operation is not allowed for the specified object |
| VKL\_OUT\_OF\_MEMORY    | there is not enough memory to execute the command     |
| VKL\_UNSUPPORTED\_CPU   | the CPU is not supported (minimum ISA is SSE4.1)      |

Possible error codes, i.e., valid named constants of type `VKLError`.

These error codes are either directly returned by some API functions, or
are recorded to be later queried by the application via

``` cpp
VKLError vklDeviceGetLastErrorCode(VKLDevice);
```

A more descriptive error message can be queried by calling

``` cpp
const char* vklDeviceGetLastErrorMsg(VKLDevice);
```

Alternatively, the application can also register a callback function of
type

``` cpp
typedef void (*VKLErrorCallback)(void *, VKLError, const char* message);
```

via

``` cpp
void vklDeviceSetErrorCallback(VKLDevice, VKLErrorFunc, void *);
```

to get notified when errors occur. Applications may be interested in
messages which Open VKL emits, whether for debugging or logging events.
Applications can register a callback function of type

``` cpp
typedef void (*VKLLogCallback)(void *, const char* message);
```

via

``` cpp
void vklDeviceSetLogCallback(VKLDevice, VKLLogCallback, void *);
```

which Open VKL will use to emit log messages. Applications can clear
either callback by passing `nullptr` instead of an actual function
pointer. By default, Open VKL uses `cout` and `cerr` to emit log and
error messages, respectively. The last parameter to
`vklDeviceSetErrorCallback` and `vklDeviceSetLogCallback` is a user data
pointer. Open VKL passes this pointer to the callback functions as the
first parameter. Note that in addition to setting the above callbacks,
this behavior can be changed via the device parameters and environment
variables described previously.

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
                   VKLDataCreationFlags dataCreationFlags,
                   size_t byteStride);
```

Data objects can be created as Open VKL owned (`dataCreationFlags =
VKL_DATA_DEFAULT`), in which the library will make a copy of the data
for its use, or shared (`dataCreationFlags = VKL_DATA_SHARED_BUFFER`),
which will try to use the passed pointer for usage. The library is
allowed to copy data when a volume is committed.

The distance between consecutive elements in `source` is given in bytes
with `byteStride`. If the provided `byteStride` is zero, then it will be
determined automatically as `sizeof(type)`. Open VKL owned data will be
compacted into a naturally-strided array on copy, regardless of the
original `byteStride`.

As with other object types, when data objects are no longer needed they
should be released via `vklRelease`.

The enum type `VKLDataType` describes the different element types that
can be represented in Open VKL. The types accepted vary per volume; see
the volume section for specifics. Valid constants are listed in the
table below.

| Type/Name                      | Description                                                                                  |
| :----------------------------- | :------------------------------------------------------------------------------------------- |
| VKL\_DEVICE                    | API device object reference                                                                  |
| VKL\_DATA                      | data reference                                                                               |
| VKL\_OBJECT                    | generic object reference                                                                     |
| VKL\_VALUE\_SELECTOR           | value selector object reference                                                              |
| VKL\_VOLUME                    | volume object reference                                                                      |
| VKL\_STRING                    | C-style zero-terminated character string                                                     |
| VKL\_CHAR, VKL\_VEC\[234\]C    | 8 bit signed character scalar and \[234\]-element vector                                     |
| VKL\_UCHAR, VKL\_VEC\[234\]UC  | 8 bit unsigned character scalar and \[234\]-element vector                                   |
| VKL\_SHORT, VKL\_VEC\[234\]S   | 16 bit unsigned integer scalar and \[234\]-element vector                                    |
| VKL\_USHORT, VKL\_VEC\[234\]US | 16 bit unsigned integer scalar and \[234\]-element vector                                    |
| VKL\_INT, VKL\_VEC\[234\]I     | 32 bit signed integer scalar and \[234\]-element vector                                      |
| VKL\_UINT, VKL\_VEC\[234\]UI   | 32 bit unsigned integer scalar and \[234\]-element vector                                    |
| VKL\_LONG, VKL\_VEC\[234\]L    | 64 bit signed integer scalar and \[234\]-element vector                                      |
| VKL\_ULONG, VKL\_VEC\[234\]UL  | 64 bit unsigned integer scalar and \[234\]-element vector                                    |
| VKL\_HALF, VKL\_VEC\[234\]H    | 16 bit half precision floating-point scalar and \[234\]-element vector (IEEE 754 `binary16`) |
| VKL\_FLOAT, VKL\_VEC\[234\]F   | 32 bit single precision floating-point scalar and \[234\]-element vector                     |
| VKL\_DOUBLE, VKL\_VEC\[234\]D  | 64 bit double precision floating-point scalar and \[234\]-element vector                     |
| VKL\_BOX\[1234\]I              | 32 bit integer box (lower + upper bounds)                                                    |
| VKL\_BOX\[1234\]F              | 32 bit single precision floating-point box (lower + upper bounds)                            |
| VKL\_LINEAR\[23\]F             | 32 bit single precision floating-point linear transform (\[23\] vectors)                     |
| VKL\_AFFINE\[23\]F             | 32 bit single precision floating-point affine transform (linear transform plus translation)  |
| VKL\_VOID\_PTR                 | raw memory address                                                                           |

Valid named constants for `VKLDataType`.

## Observers

Volumes and samplers in Open VKL may provide observers to communicate
data back to the application. Observers may be created with

``` cpp
VKLObserver vklNewSamplerObserver(VKLSampler sampler,
                                  const char *type);

VKLObserver vklNewVolumeObserver(VKLVolume volume,
                                 const char *type);
```

The object passed to `vklNew*Observer` must already be committed. Valid
observer type strings are defined by volume implementations (see section
‘Volume types’ below).

`vklNew*Observer` returns `NULL` on failure.

To access the underlying data, an observer must first be mapped using

``` cpp
const void * vklMapObserver(VKLObserver observer);
```

If this fails, the function returns `NULL`. `vklMapObserver` may fail on
observers that are already mapped. On success, the application may query
the underlying type, element size in bytes, and the number of elements
in the buffer using

``` cpp
VKLDataType vklGetObserverElementType(VKLObserver observer);
size_t vklGetObserverElementSize(VKLObserver observer);
size_t vklGetObserverNumElements(VKLObserver observer);
```

On failure, these functions return `VKL_UNKNOWN` and `0`, respectively.
Possible data types are defined by the volume that provides the observer
, as are the semantics of the observation. See section ‘Volume types’
for details.

The pointer returned by `vklMapObserver` may be cast to the type
corresponding to the value returned by `vklGetObserverElementType` to
access the observation. For example, if `vklGetObserverElementType`
returns `VKL_FLOAT`, then the pointer returned by `vklMapObserver` may
be cast to `const float *` to access up to `vklGetObserverNumElements`
consecutive values of type `float`.

Once the application has finished processing the observation, it should
unmap the observer using

``` cpp
void vklUnmapObserver(VKLObserver observer);
```

so that the observer may be mapped again.

When an observer is no longer needed, it should be released using
`vklRelease`.

The observer API is not thread safe, and these functions should not be
called concurrently on the same object.

## Volume types

Open VKL currently supports structured volumes on regular and spherical
grids; unstructured volumes with tetrahedral, wedge, pyramid, and
hexaderal primitive types; adaptive mesh refinement (AMR) volumes;
sparse VDB volumes; and particle volumes. These volumes are created with
`vklNewVolume` with the appropriate type string.

In addition to the usual `vklSet...()` and `vklCommit()` APIs, the
volume bounding box can be queried:

``` cpp
vkl_box3f vklGetBoundingBox(VKLVolume volume);
```

The number of attributes in a volume can also be queried:

``` cpp
unsigned int vklGetNumAttributes(VKLVolume volume);
```

Finally, the value range of the volume (first attribute only) can be
queried:

``` cpp
vkl_range1f vklGetValueRange(VKLVolume volume);
```

### Structured Volumes

Structured volumes only need to store the values of the samples, because
their addresses in memory can be easily computed from a 3D position. The
dimensions for all structured volume types are in units of vertices, not
cells. For example, a volume with dimensions \((x, y, z)\) will have
\((x-1, y-1, z-1)\) cells in each dimension. Voxel data provided is
assumed vertex-centered, so \(x*y*z\) values must be provided.

#### Structured Regular Volumes

A common type of structured volumes are regular grids, which are created
by passing a type string of `"structuredRegular"` to `vklNewVolume`. The
parameters understood by structured regular volumes are summarized in
the table below.

| Type                  | Name                             | Default       | Description                                                                              |
| :-------------------- | :------------------------------- | :------------ | :--------------------------------------------------------------------------------------- |
| vec3i                 | dimensions                       |               | number of voxels in each dimension \((x, y, z)\)                                         |
| VKLData VKLData\[\]   | data                             |               | VKLData object(s) of voxel data, supported types are:                                    |
|                       |                                  |               | `VKL_UCHAR`                                                                              |
|                       |                                  |               | `VKL_SHORT`                                                                              |
|                       |                                  |               | `VKL_USHORT`                                                                             |
|                       |                                  |               | `VKL_HALF`                                                                               |
|                       |                                  |               | `VKL_FLOAT`                                                                              |
|                       |                                  |               | `VKL_DOUBLE`                                                                             |
|                       |                                  |               | Multiple attributes are supported through passing an array of VKLData objects.           |
| vec3f                 | gridOrigin                       | \((0, 0, 0)\) | origin of the grid in object space                                                       |
| vec3f                 | gridSpacing                      | \((1, 1, 1)\) | size of the grid cells in object space                                                   |
| int                   | temporallyStructuredNumTimesteps |               | for temporally structured variation, number of timesteps per voxel                       |
| uint32\[\] uint64\[\] | temporallyUnstructuredIndices    |               | for temporally unstructured variation, indices to `data` time series beginning per voxel |
| float\[\]             | temporallyUnstructuredTimes      |               | for temporally unstructured variation, time values corresponding to values in `data`     |

Configuration parameters for structured regular (`"structuredRegular"`)
volumes.

Structured regular volumes support two forms of temporal variation:
temporally structured and temporally unstructured. When one of these
modes is enabled, the volume can be sampled at different times. In both
modes, time is assumed to vary between zero and one. This can be useful
for implementing renderers with motion blur, for example.

Temporally structured variation is configured through the
`temporallyStructuredNumTimesteps` parameter. This specifies how many
time steps (at least two) are provided for all voxels. Therefore, for a
volume with dimensions \((x, y, z)\), each attribute must have
\(x*y*z * temporallyStructuredNumTimesteps\) values provided in its
`data` array. The values are assumed evenly spaced over times
\([0, 1]\).

Temporally unstructured variation is configured through the
`temporallyUnstructuredIndices` and `temporallyUnstructuredTimes`
parameters, and supports differing time step counts and sample times per
voxel. `temporallyUnstructuredIndices` specifies the index ranges for
each voxel’s values in `data`, such that values for the the \(i\)th
voxel can be found in the indices
\([temporallyUnstructuredIndices[i], temporallyUnstructuredIndices[i+1])\).
Therefore `temporallyUnstructuredIndices` must have \(x*y*z + 1\)
values. `temporallyUnstructuredTimes` specifies the times corresponding
to the sample values in each attribute’s `data` array; the time values
for each voxel must be between zero and one and strictly increasing:
\(t0 < t1 < ... < tN\). To return a value at sample time t,
\(t0 <= t <= tN\), Open VKL will interpolate linearly from the two
nearest time steps. Time values outside this range are clamped
\([t0, tN]\).

The following additional parameters can be set both on
`"structuredRegular"` volumes and their sampler objects. Sampler object
parameters default to volume parameters.

| Type | Name           | Default                | Description                                                                                                     |
| :--- | :------------- | :--------------------- | :-------------------------------------------------------------------------------------------------------------- |
| int  | filter         | `VKL_FILTER_TRILINEAR` | The filter used for reconstructing the field. Use `VKLFilter` for named constants.                              |
| int  | gradientFilter | `filter`               | The filter used for reconstructing the field during gradient computations. Use `VKLFilter` for named constants. |

Configuration parameters for structured regular (`"structuredRegular"`)
volumes and their sampler objects.

#### Structured Spherical Volumes

Structured spherical volumes are also supported, which are created by
passing a type string of `"structuredSpherical"` to `vklNewVolume`. The
grid dimensions and parameters are defined in terms of radial distance
(\(r\)), inclination angle (\(\theta\)), and azimuthal angle (\(\phi\)),
conforming with the ISO convention for spherical coordinate systems. The
coordinate system and parameters understood by structured spherical
volumes are summarized below.

![Structured spherical volume coordinate system: radial distance
(\(r\)), inclination angle (\(\theta\)), and azimuthal angle
(\(\phi\)).](https://openvkl.github.io/images/structured_spherical_coords.png)

| Type                | Name        |    Default    | Description                                                                    |
| :------------------ | :---------- | :-----------: | :----------------------------------------------------------------------------- |
| vec3i               | dimensions  |               | number of voxels in each dimension \((r, \theta, \phi)\)                       |
| VKLData VKLData\[\] | data        |               | VKLData object(s) of voxel data, supported types are:                          |
|                     |             |               | `VKL_UCHAR`                                                                    |
|                     |             |               | `VKL_SHORT`                                                                    |
|                     |             |               | `VKL_USHORT`                                                                   |
|                     |             |               | `VKL_HALF`                                                                     |
|                     |             |               | `VKL_FLOAT`                                                                    |
|                     |             |               | `VKL_DOUBLE`                                                                   |
|                     |             |               | Multiple attributes are supported through passing an array of VKLData objects. |
| vec3f               | gridOrigin  | \((0, 0, 0)\) | origin of the grid in units of \((r, \theta, \phi)\); angles in degrees        |
| vec3f               | gridSpacing | \((1, 1, 1)\) | size of the grid cells in units of \((r, \theta, \phi)\); angles in degrees    |

Configuration parameters for structured spherical
(`"structuredSpherical"`) volumes.

These grid parameters support flexible specification of spheres,
hemispheres, spherical shells, spherical wedges, and so forth. The grid
extents (computed as
\([gridOrigin, gridOrigin + (dimensions - 1) * gridSpacing]\)) however
must be constrained such that:

  - \(r \geq 0\)
  - \(0 \leq \theta \leq 180\)
  - \(0 \leq \phi \leq 360\)

The following additional parameters can be set both on
`"structuredSpherical"` volumes and their sampler objects. Sampler
object parameters default to volume parameters.

| Type | Name           | Default                | Description                                                                                                     |
| :--- | :------------- | :--------------------- | :-------------------------------------------------------------------------------------------------------------- |
| int  | filter         | `VKL_FILTER_TRILINEAR` | The filter used for reconstructing the field. Use `VKLFilter` for named constants.                              |
| int  | gradientFilter | `filter`               | The filter used for reconstructing the field during gradient computations. Use `VKLFilter` for named constants. |

Configuration parameters for structured spherical
(`"structuredSpherical"`) volumes and their sampler objects.

### Adaptive Mesh Refinement (AMR) Volumes

Open VKL currently supports block-structured (Berger-Colella) AMR
volumes. Volumes are specified as a list of blocks, which exist at
levels of refinement in potentially overlapping regions. Blocks exist in
a tree structure, with coarser refinement level blocks containing finer
blocks. The cell width is equal for all blocks at the same refinement
level, though blocks at a coarser level have a larger cell width than
finer levels.

There can be any number of refinement levels and any number of blocks at
any level of refinement.

Blocks are defined by three parameters: their bounds, the refinement
level in which they reside, and the scalar data contained within each
block.

Note that cell widths are defined *per refinement level*, not per block.

AMR volumes are created by passing the type string `"amr"` to
`vklNewVolume`, and have the following parameters:

| Type        | Name         |       Default | Description                                                                                                                          |
| :---------- | :----------- | ------------: | :----------------------------------------------------------------------------------------------------------------------------------- |
| float\[\]   | cellWidth    |               | \[data\] array of each level’s cell width                                                                                            |
| box3i\[\]   | block.bounds |               | \[data\] array of each block’s bounds (in voxels)                                                                                    |
| int\[\]     | block.level  |               | \[data\] array of each block’s refinement level                                                                                      |
| VKLData\[\] | block.data   |               | \[data\] array of each block’s VKLData object containing the actual scalar voxel data. Currently only `VKL_FLOAT` data is supported. |
| vec3f       | gridOrigin   | \((0, 0, 0)\) | origin of the grid in object space                                                                                                   |
| vec3f       | gridSpacing  | \((1, 1, 1)\) | size of the grid cells in object space                                                                                               |

Configuration parameters for AMR (`"amr"`) volumes.

Note that the `gridOrigin` and `gridSpacing` parameters act just like
the structured volume equivalent, but they only modify the root
(coarsest level) of refinement.

The following additional parameters can be set both on `"amr"` volumes
and their sampler objects. Sampler object parameters default to volume
parameters.

| Type           | Name   |           Default | Description                                            |
| :------------- | :----- | ----------------: | :----------------------------------------------------- |
| `VKLAMRMethod` | method | `VKL_AMR_CURRENT` | `VKLAMRMethod` sampling method. Supported methods are: |
|                |        |                   | `VKL_AMR_CURRENT`                                      |
|                |        |                   | `VKL_AMR_FINEST`                                       |
|                |        |                   | `VKL_AMR_OCTANT`                                       |

Configuration parameters for AMR (`"AMR"`) volumes and their sampler
objects.

Open VKL’s AMR implementation was designed to cover Berger-Colella \[1\]
and Chombo \[2\] AMR data. The `method` parameter above determines the
interpolation method used when sampling the volume.

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
indices, cell start indices, cell types, and cell values.

Sampled cell values can be specified either per-vertex (`vertex.data`)
or per-cell (`cell.data`). If both arrays are set, `cell.data` takes
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

To maintain VTK data compatibility, the `index` array may be specified
with cell sizes interleaved with vertex indices in the following format:
\(n, id_1, ..., id_n, m, id_1, ..., id_m\). This alternative `index`
array layout can be enabled through the `indexPrefixed` flag (in which
case, the `cell.type` parameter should be omitted).

A binary bounding volume hierarchy (BVH) is used internally to
accelerate interval iteration. Intervals are found by intersecting BVH
nodes up to a maximum level of the tree, configurable by the
`maxIteratorDepth` parameter. Larger values of `maxIteratorDepth` lead
to smaller individual intervals (up to leaf node intersections),
yielding potentially more efficient space-skipping behavior and tighter
bounds on returned interval metadata.

Unstructured volumes are created by passing the type string
`"unstructured"` to `vklNewVolume`, and have the following parameters:

| Type                    | Name               | Default | Description                                                                                                                                             |
| :---------------------- | :----------------- | :------ | :------------------------------------------------------------------------------------------------------------------------------------------------------ |
| vec3f\[\]               | vertex.position    |         | \[data\] array of vertex positions                                                                                                                      |
| float\[\]               | vertex.data        |         | \[data\] array of vertex data values to be sampled                                                                                                      |
| uint32\[\] / uint64\[\] | index              |         | \[data\] array of indices (into the vertex array(s)) that form cells                                                                                    |
| bool                    | indexPrefixed      | false   | indicates that the `index` array is provided in a VTK-compatible format, where the indices of each cell are prefixed with the number of vertices        |
| uint32\[\] / uint64\[\] | cell.index         |         | \[data\] array of locations (into the index array), specifying the first index of each cell                                                             |
| float\[\]               | cell.data          |         | \[data\] array of cell data values to be sampled                                                                                                        |
| uint8\[\]               | cell.type          |         | \[data\] array of cell types (VTK compatible). Supported types are:                                                                                     |
|                         |                    |         | `VKL_TETRAHEDRON`                                                                                                                                       |
|                         |                    |         | `VKL_HEXAHEDRON`                                                                                                                                        |
|                         |                    |         | `VKL_WEDGE`                                                                                                                                             |
|                         |                    |         | `VKL_PYRAMID`                                                                                                                                           |
| bool                    | hexIterative       | false   | hexahedron interpolation method, defaults to fast non-iterative version which could have rendering inaccuracies may appear if hex is not parallelepiped |
| bool                    | precomputedNormals | false   | whether to accelerate by precomputing, at a cost of 12 bytes/face                                                                                       |

Configuration parameters for unstructured (`"unstructured"`) volumes.

The following additional parameters can be set both on `unstructured`
volumes and their sampler objects (sampler object parameters default to
volume parameters).

| Type | Name             | Default | Description                                                              |
| :--- | :--------------- | :------ | :----------------------------------------------------------------------- |
| int  | maxIteratorDepth | 6       | Do not descend further than to this BVH depth during interval iteration. |

Configuration parameters for unstructured (`"unstructured"`) volumes and
their sampler objects.

### VDB Volumes

VDB volumes implement a data structure that is very similar to the data
structure outlined in Museth \[1\].

The data structure is a hierarchical regular grid at its core: Nodes are
regular grids, and each grid cell may either store a constant value
(this is called a tile), or child pointers.

Nodes in VDB trees are wide: Nodes on the first level have a resolution
of 32^3 voxels by default, on the next level 16^3, and on the leaf level
8^3 voxels. All nodes on a given level have the same resolution. This
makes it easy to find the node containing a coordinate using shift
operations (cp. \[1\]).

VDB leaf nodes are implicit in Open VKL: they are stored as pointers to
user-provided data.

![Structure of `"vdb"` volumes in the default
configuration](https://openvkl.github.io/images/vdb_structure.png)

VDB volumes interpret input data as constant cells (which are then
potentially filtered). This is in contrast to `structuredRegular`
volumes, which have a vertex-centered interpretation.

The VDB implementation in Open VKL follows the following goals:

  - Efficient data structure traversal on vector architectures.

  - Enable the use of industry-standard .vdb files created through the
    OpenVDB library.

  - Compatibility with OpenVDB on a leaf data level, so that .vdb files
    may be loaded with minimal overhead.

VDB volumes are created by passing the type string `"vdb"` to
`vklNewVolume`, and have the following parameters:

| Type        | Name          | Default                            | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| :---------- | :------------ | :--------------------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| float\[\]   | indexToObject | 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 | An array of 12 values of type `float` that define the transformation from index space to object space. In index space, the grid is an axis-aligned regular grid, and leaf voxels have size (1,1,1). The first 9 values are interpreted as a row-major linear transformation matrix. The last 3 values are the translation of the grid origin.                                                                                                                                                                                            |
| uint32\[\]  | node.format   |                                    | For each input node, the data format. Currently supported are `VKL_FORMAT_TILE` for tiles, and `VKL_FORMAT_CONSTANT_ZYX` for nodes that are dense regular grids.                                                                                                                                                                                                                                                                                                                                                                         |
| uint32\[\]  | node.level    |                                    | For each input node, the level on which this node exists. Tiles may exist on levels \[1, `VKL_VDB_NUM_LEVELS-1`\], all other nodes may only exist on level `VKL_VDB_NUM_LEVELS-1`.                                                                                                                                                                                                                                                                                                                                                       |
| vec3i\[\]   | node.origin   |                                    | For each input node, the node origin index.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| VKLData\[\] | node.data     |                                    | For each input node, the attribute data. Single-attribute volumes may have one array provided per node, while multi-attribute volumes require an array per attribute for each node. Nodes with format `VKL_FORMAT_TILE` are expected to have single-entry arrays per attribute. Nodes with format `VKL_FORMAT_CONSTANT_ZYX` are expected to have arrays with `vklVdbLevelNumVoxels(level[i])` entries per attribute. `VKL_HALF` and `VKL_FLOAT` data is currently supported; all nodes for a given attribute must be the same data type. |

Configuration parameters for VDB (`"vdb"`) volumes.

The level, origin, format, and data parameters must have the same size,
and there must be at least one valid node or `commit()` will fail.

The following additional parameters can be set both on `vdb` volumes and
their sampler objects (sampler object parameters default to volume
parameters).

| Type | Name             | Default                | Description                                                                                                     |
| :--- | :--------------- | :--------------------- | :-------------------------------------------------------------------------------------------------------------- |
| int  | filter           | `VKL_FILTER_TRILINEAR` | The filter used for reconstructing the field. Use `VKLFilter` for named constants.                              |
| int  | gradientFilter   | `filter`               | The filter used for reconstructing the field during gradient computations. Use `VKLFilter` for named constants. |
| int  | maxSamplingDepth | `VKL_VDB_NUM_LEVELS`-1 | Do not descend further than to this depth during sampling.                                                      |
| int  | maxIteratorDepth | `VKL_VDB_NUM_LEVELS`-2 | Do not descend further than to this depth during iteration.                                                     |

Configuration parameters for VDB (`"vdb"`) volumes and their sampler
objects.

VDB volume objects support the following observers:

| Name      | Buffer Type | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| :-------- | ----------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| InnerNode | float\[\]   | Return an array of bounding boxes, along with value ranges, of inner nodes in the data structure. The bounding box is given in object space. For a volume with M attributes, the entries in this array are (6+2\*M)-tuples `(minX, minY, minZ, maxX, maxY, maxZ, lower_0, upper_0, lower_1, upper_1, ...)`. This is in effect a low resolution representation of the volume. The InnerNode observer can be parametrized using `int maxDepth` to control the depth at which inner nodes are returned. Note that the observer will also return leaf nodes or tiles at lower levels if they exist. |

Observers supported by VDB (`"vdb"`) volumes.

VDB sampler objects support the following observers:

| Name           | Buffer Type | Description                                                                                                                                                                                                                                    |
| :------------- | :---------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| LeafNodeAccess | uint32\[\]  | This observer returns an array with as many entries as input nodes were passed. If the input node i was accessed during traversal, then the ith entry in this array has a nonzero value. This can be used for on-demand loading of leaf nodes. |

Observers supported by sampler objects created on VDB (`"vdb"`) volumes.

#### Reconstruction filters

VDB volumes support the filter types `VKL_FILTER_NEAREST`,
`VKL_FILTER_TRILINEAR`, and `VKL_FILTER_TRICUBIC` for both `filter` and
`gradientFilter`.

Note that when `gradientFilter` is set to `VKL_FILTER_NEAREST`,
gradients are always \((0, 0, 0)\).

#### Major differences to OpenVDB

  - Open VKL implements sampling in ISPC, and can exploit wide SIMD
    architectures.

  - VDB volumes in Open VKL are read-only once committed, and designed
    for rendering only. Authoring or manipulating datasets is not in the
    scope of this implementation.

  - The only supported field types are `VKL_HALF` and `VKL_FLOAT` at
    this point. Other field types may be supported in the future. Note
    that multi-attribute volumes may be used to represent
    multi-component (e.g. vector) fields.

  - The root level in Open VKL has a single node with resolution 64^3
    (cp. \[1\]. OpenVDB uses a hash map, instead).

  - Open VKL supports four-level vdb volumes. The resolution of each
    level can be configured at compile time using CMake variables.
    
      - `VKL_VDB_LOG_RESOLUTION_0` sets the base 2 logarithm of the root
        level resolution. This variable defaults to 6, which means that
        the root level has a resolution of \((2^6)^3 = 64^3\).
      - `VKL_VDB_LOG_RESOLUTION_1` and `VKL_VDB_LOG_RESOLUTION_2`
        default to 5 and 4, respectively. This matches the default Open
        VDB resolution for inner levels.
      - `VKL_VDB_LOG_RESOLUTION_3` set the base 2 logarithm of the leaf
        level resolution, and defaults to 3. Therefore, leaf nodes have
        a resolution of \(8^3\) voxels. Again, this matches the Open VDB
        default. The default settings lead to a domain resolution of
        \(2^18^3=262144^3\) voxels.

#### Loading OpenVDB .vdb files

Files generated with OpenVDB can be loaded easily since Open VKL `vdb`
volumes implement the same leaf data layout. This means that OpenVDB
leaf data pointers can be passed to Open VKL using shared data buffers,
avoiding copy operations.

An example of this can be found in
`vdb_util/include/openvkl/OpenVdbGrid.h`, where the class
`OpenVdbFloatGrid` encapsulates the necessary operations. This class is
also accessible through the `vklExamples` application using the `-file`
and `-field` command line arguments.

To use this example feature, compile Open VKL with `OpenVDB_ROOT`
pointing to the OpenVDB prefix.

1.  Museth, K. VDB: High-Resolution Sparse Volumes with Dynamic
    Topology. ACM Transactions on Graphics 32(3), 2013. DOI:
    10.1145/2487228.2487235

### Particle Volumes

Particle volumes consist of a set of points in space. Each point has a
position, a radius, and a weight typically associated with an attribute.
A radial basis function defines the contribution of that particle.
Currently, we use the Gaussian radial basis function,

phi(P) = w \* exp( -0.5 \* ((P - p) / r)^2 )

where P is the particle position, p is the sample position, r is the
radius and w is the weight.

At each sample, the scalar field value is then computed as the sum of
each radial basis function phi, for each particle that overlaps it.

The Open VKL implementation is similar to direct evaluation of samples
in Reda et al.\[2\]. It uses an Embree-built BVH with a custom
traversal, similar to the method in \[1\].

Similar to unstructured volumes, a binary BVH is used internally to
accelerate interval iteration. Intervals are found by intersecting BVH
nodes up to a maximum level of the tree, configurable by the
`maxIteratorDepth` parameter.

Particle volumes are created by passing the type string `"particle"` to
`vklNewVolume`, and have the following parameters:

| Type      | Name                    | Default | Description                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| :-------- | :---------------------- | :------ | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| vec3f\[\] | particle.position       |         | \[data\] array of particle positions                                                                                                                                                                                                                                                                                                                                                                                                               |
| float\[\] | particle.radius         |         | \[data\] array of particle radii                                                                                                                                                                                                                                                                                                                                                                                                                   |
| float\[\] | particle.weight         | null    | \[data\] (optional) array of particle weights, specifying the height of the kernel.                                                                                                                                                                                                                                                                                                                                                                |
| float     | radiusSupportFactor     | 3.0     | The multipler of the particle radius required for support. Larger radii ensure smooth results at the cost of performance. In the Gaussian kernel, the the radius is one standard deviation (sigma), so a `radiusSupportFactor` of 3 corresponds to 3\*sigma.                                                                                                                                                                                       |
| float     | clampMaxCumulativeValue | 0       | The maximum cumulative value possible, set by user. All cumulative values will be clamped to this, and further traversal (RBF summation) of particle contributions will halt when this value is reached. A value of zero or less turns this off.                                                                                                                                                                                                   |
| bool      | estimateValueRanges     | true    | Enable heuristic estimation of value ranges which are used in internal acceleration structures for interval and hit iterators, as well as for determining the volume’s overall value range. When set to `false`, the user *must* specify `clampMaxCumulativeValue`, and all value ranges will be assumed \[0, `clampMaxCumulativeValue`\]. Disabling this may improve volume commit time, but will make interval and hit iteration less efficient. |

Configuration parameters for particle (`"particle"`) volumes.

The following additional parameters can be set both on `particle`
volumes and their sampler objects (sampler object parameters default to
volume parameters).

| Type | Name             | Default | Description                                                              |
| :--- | :--------------- | :------ | :----------------------------------------------------------------------- |
| int  | maxIteratorDepth | 6       | Do not descend further than to this BVH depth during interval iteration. |

Configuration parameters for particle (`"particle"`) volumes and their
sampler objects.

1.  Knoll, A., Wald, I., Navratil, P., Bowen, A., Reda, K., Papka, M.E.
    and Gaither, K. (2014), RBF Volume Ray Casting on Multicore and
    Manycore CPUs. Computer Graphics Forum, 33: 71-80.
    doi:10.1111/cgf.12363

2.  K. Reda, A. Knoll, K. Nomura, M. E. Papka, A. E. Johnson and J.
    Leigh, “Visualizing large-scale atomistic simulations in
    ultra-resolution immersive environments,” 2013 IEEE Symposium on
    Large-Scale Data Analysis and Visualization (LDAV), Atlanta, GA,
    2013, pp. 59-65.

## Sampler Objects

Computing the value of a volume at an object space coordinate is done
using the sampling API, and sampler objects. Sampler objects can be
created using

``` cpp
VKLSampler vklNewSampler(VKLVolume volume);
```

Sampler objects may then be parametrized with traversal parameters.
Available parameters are defined by volumes, and are a subset of the
volume parameters. As an example, `filter` can be set on both `vdb`
volumes and their sampler objects. The volume parameter is used as the
default for sampler objects. The sampler object parameter provides an
override per ray. More detail on parameters can be found in the sections
on volumes. Use `vklCommit()` to commit parameters to the sampler
object.

## Sampling

The scalar API takes a volume and coordinate, and returns a float value.
NaN is returned for probe points outside the volume. The attribute index
selects the scalar attribute of interest; not all volumes support
multiple attributes. The time value, which must be between 0 and 1,
specifies the sampling time. For temporally constant volumes, this value
has no effect.

``` cpp
float vklComputeSample(VKLSampler sampler,
                       const vkl_vec3f *objectCoordinates,
                       unsigned int attributeIndex,
                       float time);
```

Vector versions allow sampling at 4, 8, or 16 positions at once.
Depending on the machine type and Open VKL device implementation, these
can give greater performance. An active lane mask `valid` is passed in
as an array of integers; set 0 for lanes to be ignored, -1 for active
lanes. An array of time values corresponding to each object coordinate
may be provided; a `NULL` value indicates all times are zero.

``` cpp
void vklComputeSample4(const int *valid,
                       VKLSampler sampler,
                       const vkl_vvec3f4 *objectCoordinates,
                       float *samples,
                       unsigned int attributeIndex,
                       const float *times);

void vklComputeSample8(const int *valid,
                       VKLSampler sampler,
                       const vkl_vvec3f8 *objectCoordinates,
                       float *samples,
                       unsigned int attributeIndex,
                       const float *times);

void vklComputeSample16(const int *valid,
                        VKLSampler sampler,
                        const vkl_vvec3f16 *objectCoordinates,
                        float *samples,
                        unsigned int attributeIndex,
                        const float *times);
```

A stream version allows sampling an arbitrary number of positions at
once. While the vector version requires coordinates to be provided in a
structure-of-arrays layout, the stream version allows coordinates to be
provided in an array-of-structures layout. Thus, the stream API can be
used to avoid reformatting of data by the application. As with the
vector versions, the stream API can give greater performance than the
scalar API.

``` cpp
  void vklComputeSampleN(VKLSampler sampler,
                         unsigned int N,
                         const vkl_vec3f *objectCoordinates,
                         float *samples,
                         unsigned int attributeIndex,
                         const float *times);
```

All of the above sampling APIs can be used, regardless of the device’s
native SIMD width.

### Sampling Multiple Attributes

Open VKL provides additional APIs for sampling multiple scalar
attributes in a single call through the `vklComputeSampleM*()`
interfaces. Beyond convenience, these can give improved performance
relative to the single attribute sampling APIs. As with the single
attribute APIs, sampling time values may be specified; note that these
are provided per object coordinate only (rather than separately per
attribute).

A scalar API supports sampling `M` attributes specified by
`attributeIndices` on a single object space coordinate:

``` cpp
void vklComputeSampleM(VKLSampler sampler,
                       const vkl_vec3f *objectCoordinates,
                       float *samples,
                       unsigned int M,
                       const unsigned int *attributeIndices,
                       float time);
```

Vector versions allow sampling at 4, 8, or 16 positions at once across
the `M` attributes:

``` cpp
void vklComputeSampleM4(const int *valid,
                        VKLSampler sampler,
                        const vkl_vvec3f4 *objectCoordinates,
                        float *samples,
                        unsigned int M,
                        const unsigned int *attributeIndices,
                        const float *times);

void vklComputeSampleM8(const int *valid,
                        VKLSampler sampler,
                        const vkl_vvec3f8 *objectCoordinates,
                        float *samples,
                        unsigned int M,
                        const unsigned int *attributeIndices,
                        const float *times);

void vklComputeSampleM16(const int *valid,
                         VKLSampler sampler,
                         const vkl_vvec3f16 *objectCoordinates,
                         float *samples,
                         unsigned int M,
                         const unsigned int *attributeIndices,
                         const float *times);
```

The `[4, 8, 16] * M` sampled values are populated in the `samples` array
in a structure-of-arrays layout, with all values for each attribute
provided in sequence. That is, sample values `s_m,n` for the `m`th
attribute and `n`th object coordinate will be populated as

``` cpp
samples = [s_0,0,   s_0,1,   ..., s_0,N-1,
           s_1,0,   s_1,1,   ..., s_1,N-1,
           ...,
           s_M-1,0, s_M-1,1, ..., s_M-1,N-1]
```

A stream version allows sampling an arbitrary number of positions at
once across the `M` attributes. As with single attribute stream
sampling, the `N` coordinates are provided in an array-of-structures
layout.

``` cpp
void vklComputeSampleMN(VKLSampler sampler,
                        unsigned int N,
                        const vkl_vec3f *objectCoordinates,
                        float *samples,
                        unsigned int M,
                        const unsigned int *attributeIndices,
                        const float *times);
```

The `M * N` sampled values are populated in the `samples` array in an
array-of-structures layout, with all attribute values for each
coordinate provided in sequence as

``` cpp
samples = [s_0,0,   s_1,0,   ..., s_M-1,0,
           s_0,1,   s_1,1,   ..., s_M-1,1,
           ...,
           s_0,N-1, s_1,N-1, ..., s_M-1,N-1]
```

All of the above sampling APIs can be used, regardless of the device’s
native SIMD width.

## Gradients

In a very similar API to `vklComputeSample`, `vklComputeGradient`
queries the value gradient at an object space coordinate. Again, a
scalar API, now returning a vec3f instead of a float. NaN values are
returned for points outside the volume. The time value, which must be
between 0 and 1, specifies the sampling time. For temporally constant
volumes, this value has no effect.

``` cpp
vkl_vec3f vklComputeGradient(VKLSampler sampler,
                             const vkl_vec3f *objectCoordinates,
                             unsigned int attributeIndex,
                             float time);
```

Vector versions are also provided:

``` cpp
void vklComputeGradient4(const int *valid,
                         VKLSampler sampler,
                         const vkl_vvec3f4 *objectCoordinates,
                         vkl_vvec3f4 *gradients,
                         unsigned int attributeIndex,
                         const float *times);

void vklComputeGradient8(const int *valid,
                         VKLSampler sampler,
                         const vkl_vvec3f8 *objectCoordinates,
                         vkl_vvec3f8 *gradients,
                         unsigned int attributeIndex,
                         const float *times);

void vklComputeGradient16(const int *valid,
                          VKLSampler sampler,
                          const vkl_vvec3f16 *objectCoordinates,
                          vkl_vvec3f16 *gradients,
                          unsigned int attributeIndex,
                          const float *times);
```

Finally, a stream version is provided:

``` cpp
void vklComputeGradientN(VKLSampler sampler,
                         unsigned int N,
                         const vkl_vec3f *objectCoordinates,
                         vkl_vec3f *gradients,
                         unsigned int attributeIndex,
                         const float *times);
```

All of the above gradient APIs can be used, regardless of the device’s
native SIMD width.

## Iterators

Open VKL has APIs to search for particular volume values along a ray.
Queries can be for ranges of volume values (`vklIterateInterval`) or for
particular values (`vklIterateHit`). Only the first volume attribute is
currently considered in the iterator APIs.

The desired values are set in a `VKLValueSelector`, which needs to be
created, filled in with values, and then committed.

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
must be initialized with `vklInitIntervalIterator`.

``` cpp
VKLIntervalIterator vklInitIntervalIterator(VKLSampler sampler,
                                            const vkl_vec3f *origin,
                                            const vkl_vec3f *direction,
                                            const vkl_range1f *tRange,
                                            VKLValueSelector valueSelector,
                                            void *buffer);

VKLIntervalIterator4 vklInitIntervalIterator4(const int *valid,
                                              VKLSampler sampler,
                                              const vkl_vvec3f4 *origin,
                                              const vkl_vvec3f4 *direction,
                                              const vkl_vrange1f4 *tRange,
                                              VKLValueSelector valueSelector,
                                              void *buffer);

VKLIntervalIterator8 vklInitIntervalIterator8(const int *valid,
                                              VKLSampler sampler,
                                              const vkl_vvec3f8 *origin,
                                              const vkl_vvec3f8 *direction,
                                              const vkl_vrange1f8 *tRange,
                                              VKLValueSelector valueSelector,
                                              void *buffer);

VKLIntervalIterator16 vklInitIntervalIterator16(const int *valid,
                                                VKLSampler sampler,
                                                const vkl_vvec3f16 *origin,
                                                const vkl_vvec3f16 *direction,
                                                const vkl_vrange1f16 *tRange,
                                                VKLValueSelector valueSelector,
                                                void *buffer);
```

Open VKL places the iterator struct into a user-provided buffer, and the
returned handle is essentially a pointer into this buffer. This means
that the iterator handle must not be used after the buffer ceases to
exist. Copying iterator buffers is currently not supported.

The required size, in bytes, of the buffer can be queried with

``` cpp
size_t vklGetIntervalIteratorSize(VKLSampler sampler);

size_t vklGetIntervalIteratorSize4(VKLSampler sampler);

size_t vklGetIntervalIteratorSize8(VKLSampler sampler);

size_t vklGetIntervalIteratorSize16(VKLSampler sampler);
```

The values these functions return may change depending on the parameters
set on `sampler`.

Open VKL also provides a conservative maximum size over all volume types
as a preprocessor definition (`VKL_MAX_INTERVAL_ITERATOR_SIZE`). This is
particularly useful for stack-based allocation in ISPC. Open VKL will
attempt to detect the native vector width using `TARGET_WIDTH`, which is
defined in recent versions of ISPC.

Intervals can then be processed by calling `vklIterateInterval` as long
as the returned lane masks indicates that the iterator is still within
the volume:

``` cpp
int vklIterateInterval(VKLIntervalIterator iterator,
                       VKLInterval *interval);

void vklIterateInterval4(const int *valid,
                         VKLIntervalIterator4 iterator,
                         VKLInterval4 *interval,
                         int *result);

void vklIterateInterval8(const int *valid,
                         VKLIntervalIterator8 iterator,
                         VKLInterval8 *interval,
                         int *result);

void vklIterateInterval16(const int *valid,
                          VKLIntervalIterator16 iterator,
                          VKLInterval16 *interval,
                          int *result);
```

The intervals returned have a t-value range, a value range, and a
`nominalDeltaT` which is approximately the step size (in units of ray
direction) that should be used to walk through the interval, if desired.
The number and length of intervals returned is volume type
implementation dependent. There is currently no way of requesting a
particular splitting.

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
isosurfaces. In contrast to interval iterators, time value(s) may be
provided to specify the sampling time. These values must be between 0
and 1; for the vector versions, a `NULL` value indicates all times are
zero. For temporally constant volumes, the time values have no effect.
Again, a user allocated buffer must be provided, and a `VKLHitIterator`
of the desired width must be initialized:

``` cpp
VKLHitIterator vklInitHitIterator(VKLSampler sampler,
                                  const vkl_vec3f *origin,
                                  const vkl_vec3f *direction,
                                  const vkl_range1f *tRange,
                                  float time,
                                  VKLValueSelector valueSelector,
                                  void *buffer);

VKLHitIterator4 vklInitHitIterator4(const int *valid,
                         VKLSampler sampler,
                         const vkl_vvec3f4 *origin,
                         const vkl_vvec3f4 *direction,
                         const vkl_vrange1f4 *tRange,
                         const float *times,
                         VKLValueSelector valueSelector,
                         void *buffer);

VKLHitIterator8 vklInitHitIterator8(const int *valid,
                         VKLSampler sampler,
                         const vkl_vvec3f8 *origin,
                         const vkl_vvec3f8 *direction,
                         const vkl_vrange1f8 *tRange,
                         const float *times,
                         VKLValueSelector valueSelector,
                         void *buffer);

VKLHitIterator16 vklInitHitIterator16(const int *valid,
                          VKLSampler sampler,
                          const vkl_vvec3f16 *origin,
                          const vkl_vvec3f16 *direction,
                          const vkl_vrange1f16 *tRange,
                          const float *times,
                          VKLValueSelector valueSelector,
                          void *buffer);
```

Buffer size can be queried with

``` cpp
size_t vklGetHitIteratorSize(VKLSampler sampler);

size_t vklGetHitIteratorSize4(VKLSampler sampler);

size_t vklGetHitIteratorSize8(VKLSampler sampler);

size_t vklGetHitIteratorSize16(VKLSampler sampler);
```

Open VKL also provides the macro `VKL_MAX_HIT_ITERATOR_SIZE` as a
conservative estimate.

Hits are then queried by looping a call to `vklIterateHit` as long as
the returned lane mask indicates that the iterator is still within the
volume.

``` cpp
int vklIterateHit(VKLHitIterator iterator, VKLHit *hit);

void vklIterateHit4(const int *valid,
                    VKLHitIterator4 iterator,
                    VKLHit4 *hit,
                    int *result);

void vklIterateHit8(const int *valid,
                    VKLHitIterator8 iterator,
                    VKLHit8 *hit,
                    int *result);

void vklIterateHit16(const int *valid,
                     VKLHitIterator16 iterator,
                     VKLHit16 *hit,
                     int *result);
```

Returned hits consist of a t-value, a volume value (equal to one of the
requested values specified in the value selector), and an (object space)
epsilon value estimating the error of the intersection:

``` cpp
typedef struct
{
  float t;
  float sample;
  float epsilon;
} VKLHit;

typedef struct
{
  float t[4];
  float sample[4];
  float epsilon[4];
} VKLHit4;

typedef struct
{
  float t[8];
  float sample[8];
  float epsilon[8];
} VKLHit8;

typedef struct
{
  float t[16];
  float sample[16];
  float epsilon[16];
} VKLHit16;
```

For both interval and hit iterators, only the vector-wide API for the
native SIMD width (determined via `vklGetNativeSIMDWidth` can be called.
The scalar versions are always valid. This restriction will likely be
lifted in the future.

# Performance Recommendations

## MXCSR control and status register

It is strongly recommended to have the `Flush to Zero` and `Denormals
are Zero` mode of the MXCSR control and status register enabled for each
thread before calling the sampling, gradient, or interval API functions.
Otherwise, under some circumstances special handling of denormalized
floating point numbers can significantly reduce application and Open VKL
performance. The device parameter `flushDenormals` or environment
variable `OPENVKL_FLUSH_DENORMALS` can be used to toggle this mode; by
default it is enabled. Alternatively, when using Open VKL together with
the Intel® Threading Building Blocks, it is sufficient to execute the
following code at the beginning of the application main thread (before
the creation of the `tbb::task_scheduler_init` object):

``` cpp
#include <xmmintrin.h>
#include <pmmintrin.h>
...
_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
```

If using a different tasking system, make sure each thread calling into
Open VKL has the proper mode set.

## Iterator Allocation

`vklInitIntervalIterator` and `vklInitHitIterator` expect a user
allocated buffer. While this buffer can be allocated by any means, we
expect iterators to be used in inner loops and advise against heap
allocation in that case. Applications may provide high performance
memory pools, but as a preferred alternative we recommend stack
allocated buffers.

In C99, variable length arrays provide an easy way to achieve this:

``` cpp
const size_t bufferSize = vklGetIntervalIteratorSize(sampler);
char buffer[bufferSize];
```

Note that the call to `vklGetIntervalIteratorSize` or
`vklGetHitIteratorSize` should not appear in an inner loop as it is
relatively costly. The return value depends on the volume type, target
architecture, and parameters to `sampler`.

In C++, variable length arrays are not part of the standard. Here, users
may rely on `alloca` and similar functions:

``` cpp
#include <alloca.h>
const size_t bufferSize = vklGetIntervalIteratorSize(sampler);
char *buffer = alloca(bufferSize);
```

Users should understand the implications of `alloca`. In particular,
`alloca` does check available stack space and may result in stack
overflow. `buffer` also becomes invalid at the end of the scope. As one
consequence, it cannot be returned from a function. On Windows,
`_malloca` is a safer option that performs additional error checking,
but requires the use of `_freea`.

In ISPC, variable length or `alloca` do not exist. Applications may
instead rely on the `VKL_MAX_INTERVAL_ITERATOR_SIZE` and
`VKL_MAX_HIT_ITERATOR_SIZE` macros:

``` cpp
uniform unsigned int8 buffer[VKL_MAX_INTERVAL_ITERATOR_SIZE];
```

These values are majorants over all devices and volume types. Note that
Open VKL attempts to detect the target SIMD width using `TARGET_WIDTH`,
returning smaller buffer sizes for narrow architectures. However, Open
VKL may fall back to the largest buffer size over all targets.

## Multi-attribute Volume Data Layout

Open VKL provides flexible managed data APIs that allow applications to
specify input data in various formats and layouts. When shared buffers
are used (`dataCreationFlags = VKL_DATA_SHARED_BUFFER`), Open VKL will
use the application-owned memory directly, respecting the input data
layout. Shared buffers therefore allow applications to strategically
select the best layout for multi-attribute volume data and expected
sampling behavior.

For volume attributes that are sampled individually (e.g. using
`vklComputeSample[4,8,16,N]()`), it is recommended to use a
structure-of-arrays layout. That is, each attribute’s data should be
compact in contiguous memory. This can be accomplished by simply using
Open VKL owned data objects (`dataCreationFlags = VKL_DATA_DEFAULT`), or
by using a natural `byteStride` for shared buffers.

For volume attributes that are sampled simultaneously (e.g. using
`vklComputeSampleM[4,8,16,N]()`), it is recommended to use an
array-of-structures layout. That is, data for these attributes should be
provided per voxel in a contiguous layout. This is accomplished using
shared buffers for each attribute with appropriate byte strides. For
example, for a three attribute structured volume representing a velocity
field, the data can be provided as:

``` cpp
// used in Open VKL shared buffers, so must not be freed by application
std::vector<vkl_vec3f> velocities(numVoxels);

for (auto &v : velocities) {
  v.x = ...;
  v.y = ...;
  v.z = ...;
}

std::vector<VKLData> attributes;

attributes.push_back(vklNewData(velocities.size(),
                                VKL_FLOAT,
                                &velocities[0].x,
                                VKL_DATA_SHARED_BUFFER,
                                sizeof(vkl_vec3f)));

attributes.push_back(vklNewData(velocities.size(),
                                VKL_FLOAT,
                                &velocities[0].y,
                                VKL_DATA_SHARED_BUFFER,
                                sizeof(vkl_vec3f)));

attributes.push_back(vklNewData(velocities.size(),
                                VKL_FLOAT,
                                &velocities[0].z,
                                VKL_DATA_SHARED_BUFFER,
                                sizeof(vkl_vec3f)));

VKLData attributesData =
    vklNewData(attributes.size(), VKL_DATA, attributes.data());

for (auto &attribute : attributes)
  vklRelease(attribute);

VKLVolume volume = vklNewVolume("structuredRegular");

vklSetData(volume, "data", attributesData);
vklRelease(attributesData);

// set other volume parameters...

vklCommit(volume);
```

These are general recommendations for common scenarios; it is still
recommended to evaluate performance of different volume data layouts for
your application’s particular use case.

# Examples

Open VKL ships with simple tutorial applications demonstrating the basic
usage of the API, as well as full renderers showing recommended usage.

## Tutorials

Simple tutorials can be found in the
[`examples/`](https://github.com/openvkl/openvkl/tree/master/examples)
directory. These are:

  - [`vklTutorial.c`](https://github.com/openvkl/openvkl/blob/master/examples/vklTutorial.c):
    usage of the C API
  - [`vklTutorialISPC.cpp`](https://github.com/openvkl/openvkl/blob/master/examples/ispc/vklTutorialISPC.cpp)
    and
    [`vklTutorialISPC.ispc`](https://github.com/openvkl/openvkl/blob/master/examples/ispc/vklTutorialISPC.ispc)
    : combined usage of the C and ISPC APIs

## Interactive examples

Open VKL also ships with an interactive example application,
[`vklExamples`](https://github.com/openvkl/openvkl/blob/master/examples/interactive/vklExamples.cpp).
This interactive viewer demonstrates multiple example renderers
including a path tracer, isosurface renderer (using hit iterators), and
ray marcher. The viewer UI supports switching between renderers
interactively.

Each renderer has both a C++ and ISPC implementation showing recommended
API usage. These implementations are available in the
[`examples/interactive/renderers/`](https://github.com/openvkl/openvkl/tree/master/examples/interactive/renderers)
directory.

![`vklExamples` interactive example
application](https://openvkl.github.io/images/vklExamples.png)

## vklTutorial source

For quick reference, the contents of
[`vklTutorial.c`](https://github.com/openvkl/openvkl/blob/master/examples/vklTutorial.c)
are shown below.

``` cpp

#include <openvkl/openvkl.h>
#include <stdio.h>

#if defined(_MSC_VER)
#include <malloc.h>   // _malloca
#include <windows.h>  // Sleep
#endif

void demoScalarAPI(VKLVolume volume)
{
  printf("demo of 1-wide API\n");

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  // bounding box
  vkl_box3f bbox = vklGetBoundingBox(volume);
  printf("\tbounding box\n");
  printf("\t\tlower = %f %f %f\n", bbox.lower.x, bbox.lower.y, bbox.lower.z);
  printf("\t\tupper = %f %f %f\n\n", bbox.upper.x, bbox.upper.y, bbox.upper.z);

  // number of attributes
  unsigned int numAttributes = vklGetNumAttributes(volume);
  printf("\tnum attributes = %d\n\n", numAttributes);

  // value range (first attribute)
  vkl_range1f valueRange = vklGetValueRange(volume);
  printf("\tvalue range (first attribute) = (%f %f)\n\n",
         valueRange.lower,
         valueRange.upper);

  // coordinate for sampling / gradients
  vkl_vec3f coord = {1.f, 2.f, 3.f};
  printf("\tcoord = %f %f %f\n\n", coord.x, coord.y, coord.z);

  // sample, gradient (first attribute)
  unsigned int attributeIndex = 0;
  float time                  = 0.f;
  float sample   = vklComputeSample(sampler, &coord, attributeIndex, time);
  vkl_vec3f grad = vklComputeGradient(sampler, &coord, attributeIndex, time);
  printf("\tsampling and gradient computation (first attribute)\n");
  printf("\t\tsample = %f\n", sample);
  printf("\t\tgrad   = %f %f %f\n\n", grad.x, grad.y, grad.z);

  // sample (multiple attributes)
  unsigned int M                  = 3;
  unsigned int attributeIndices[] = {0, 1, 2};
  float samples[3];
  vklComputeSampleM(sampler, &coord, samples, M, attributeIndices, time);
  printf("\tsampling (multiple attributes)\n");
  printf("\t\tsamples = %f %f %f\n\n", samples[0], samples[1], samples[2]);

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
  vkl_vec3f rayOrigin    = {0, 1, 1};
  vkl_vec3f rayDirection = {1, 0, 0};
  vkl_range1f rayTRange  = {0, 200};
  printf("\trayOrigin = %f %f %f\n", rayOrigin.x, rayOrigin.y, rayOrigin.z);
  printf("\trayDirection = %f %f %f\n",
         rayDirection.x,
         rayDirection.y,
         rayDirection.z);
  printf("\trayTRange = %f %f\n", rayTRange.lower, rayTRange.upper);

  // interval iteration. This is scoped
  {
    // Note: buffer will cease to exist at the end of this scope.
#if defined(_MSC_VER)
    // MSVC does not support variable length arrays, but provides a
    // safer version of alloca.
    char *buffer = _malloca(vklGetIntervalIteratorSize(sampler));
#else
    char buffer[vklGetIntervalIteratorSize(sampler)];
#endif
    VKLIntervalIterator intervalIterator = vklInitIntervalIterator(
        sampler, &rayOrigin, &rayDirection, &rayTRange, selector, buffer);

    printf("\n\tinterval iterator for value ranges {%f %f} {%f %f}\n",
           ranges[0].lower,
           ranges[0].upper,
           ranges[1].lower,
           ranges[1].upper);

    for (;;) {
      VKLInterval interval;
      int result = vklIterateInterval(intervalIterator, &interval);
      if (!result)
        break;
      printf(
          "\t\ttRange (%f %f)\n\t\tvalueRange (%f %f)\n\t\tnominalDeltaT "
          "%f\n\n",
          interval.tRange.lower,
          interval.tRange.upper,
          interval.valueRange.lower,
          interval.valueRange.upper,
          interval.nominalDeltaT);
    }
#if defined(_MSC_VER)
    _freea(buffer);
#endif
  }

  // hit iteration
  {
#if defined(_MSC_VER)
    // MSVC does not support variable length arrays, but provides a
    // safer version of alloca.
    char *buffer = _malloca(vklGetHitIteratorSize(sampler));
#else
    char buffer[vklGetHitIteratorSize(sampler)];
#endif
    VKLHitIterator hitIterator = vklInitHitIterator(
        sampler, &rayOrigin, &rayDirection, &rayTRange, time, selector, buffer);

    printf("\thit iterator for values %f %f\n", values[0], values[1]);

    for (;;) {
      VKLHit hit;
      int result = vklIterateHit(hitIterator, &hit);
      if (!result)
        break;
      printf("\t\tt %f\n\t\tsample %f\n\t\tepsilon %f\n\n",
             hit.t,
             hit.sample,
             hit.epsilon);
    }
#if defined(_MSC_VER)
    _freea(buffer);
#endif
  }

  vklRelease(selector);
  vklRelease(sampler);
}

void demoVectorAPI(VKLVolume volume)
{
  printf("demo of 4-wide API (8- and 16- follow the same pattern)\n");

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  // structure-of-array layout
  vkl_vvec3f4 coord4;
  int valid[4];
  for (int i = 0; i < 4; i++) {
    coord4.x[i] = i * 3 + 0;
    coord4.y[i] = i * 3 + 1;
    coord4.z[i] = i * 3 + 2;
    valid[i]    = -1;  // valid mask: 0 = not valid, -1 = valid
  }

  for (int i = 0; i < 4; i++) {
    printf(
        "\tcoord[%d] = %f %f %f\n", i, coord4.x[i], coord4.y[i], coord4.z[i]);
  }

  // sample, gradient (first attribute)
  unsigned int attributeIndex = 0;
  float time4[4]              = {0.f};
  float sample4[4];
  vkl_vvec3f4 grad4;
  vklComputeSample4(valid, sampler, &coord4, sample4, attributeIndex, time4);
  vklComputeGradient4(valid, sampler, &coord4, &grad4, attributeIndex, time4);

  printf("\n\tsampling and gradient computation (first attribute)\n");

  for (int i = 0; i < 4; i++) {
    printf("\t\tsample[%d] = %f\n", i, sample4[i]);
    printf(
        "\t\tgrad[%d]   = %f %f %f\n", i, grad4.x[i], grad4.y[i], grad4.z[i]);
  }

  // sample (multiple attributes)
  unsigned int M                  = 3;
  unsigned int attributeIndices[] = {0, 1, 2};
  float samples[3 * 4];
  vklComputeSampleM4(
      valid, sampler, &coord4, samples, M, attributeIndices, time4);

  printf("\n\tsampling (multiple attributes)\n");

  printf("\t\tsamples = ");

  for (unsigned int j = 0; j < M; j++) {
    printf("%f %f %f %f\n",
           samples[j * 4 + 0],
           samples[j * 4 + 1],
           samples[j * 4 + 2],
           samples[j * 4 + 3]);
    printf("\t\t          ");
  }

  printf("\n");

  vklRelease(sampler);
}

void demoStreamAPI(VKLVolume volume)
{
  printf("demo of stream API\n");

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  // array-of-structure layout; arbitrary stream lengths are supported
  vkl_vec3f coord[5];

  for (int i = 0; i < 5; i++) {
    coord[i].x = i * 3 + 0;
    coord[i].y = i * 3 + 1;
    coord[i].z = i * 3 + 2;
  }

  for (int i = 0; i < 5; i++) {
    printf("\tcoord[%d] = %f %f %f\n", i, coord[i].x, coord[i].y, coord[i].z);
  }

  // sample, gradient (first attribute)
  printf("\n\tsampling and gradient computation (first attribute)\n");
  unsigned int attributeIndex = 0;
  float time [5]              = {0.f};
  float sample[5];
  vkl_vec3f grad[5];
  vklComputeSampleN(sampler, 5, coord, sample, attributeIndex, time);
  vklComputeGradientN(sampler, 5, coord, grad, attributeIndex, time);

  for (int i = 0; i < 5; i++) {
    printf("\t\tsample[%d] = %f\n", i, sample[i]);
    printf("\t\tgrad[%d]   = %f %f %f\n", i, grad[i].x, grad[i].y, grad[i].z);
  }

  // sample (multiple attributes)
  unsigned int M                  = 3;
  unsigned int attributeIndices[] = {0, 1, 2};
  float samples[3 * 5];
  vklComputeSampleMN(sampler, 5, coord, samples, M, attributeIndices, time);

  printf("\n\tsampling (multiple attributes)\n");

  printf("\t\tsamples = ");

  for (int i = 0; i < 5; i++) {
    for (unsigned int j = 0; j < M; j++) {
      printf("%f ", samples[i * M + j]);
    }
    printf("\n\t\t          ");
  }

  printf("\n");

  vklRelease(sampler);
}

int main()
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  const int dimensions[] = {128, 128, 128};

  const int numVoxels = dimensions[0] * dimensions[1] * dimensions[2];

  const int numAttributes = 3;

  VKLVolume volume = vklNewVolume("structuredRegular");
  vklSetVec3i(
      volume, "dimensions", dimensions[0], dimensions[1], dimensions[2]);
  vklSetVec3f(volume, "gridOrigin", 0, 0, 0);
  vklSetVec3f(volume, "gridSpacing", 1, 1, 1);

  float *voxels = malloc(numVoxels * sizeof(float));

  if (!voxels) {
    printf("failed to allocate voxel memory!\n");
    return 1;
  }

  // volume attribute 0: x-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)i;

  VKLData data0 = vklNewData(numVoxels, VKL_FLOAT, voxels, VKL_DATA_DEFAULT, 0);

  // volume attribute 1: y-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)j;

  VKLData data1 = vklNewData(numVoxels, VKL_FLOAT, voxels, VKL_DATA_DEFAULT, 0);

  // volume attribute 2: z-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)k;

  VKLData data2 = vklNewData(numVoxels, VKL_FLOAT, voxels, VKL_DATA_DEFAULT, 0);

  VKLData attributes[] = {data0, data1, data2};

  VKLData attributesData =
      vklNewData(numAttributes, VKL_DATA, attributes, VKL_DATA_DEFAULT, 0);

  vklRelease(data0);
  vklRelease(data1);
  vklRelease(data2);

  vklSetData(volume, "data", attributesData);
  vklRelease(attributesData);

  vklCommit(volume);

  demoScalarAPI(volume);
  demoVectorAPI(volume);
  demoStreamAPI(volume);

  vklRelease(volume);

  vklShutdown();

  free(voxels);

  printf("complete.\n");

#if defined(_MSC_VER)
  // On Windows, sleep for a few seconds so the terminal window doesn't close
  // immediately.
  Sleep(3000);
#endif

  return 0;
}
```

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
    (ISPC)](http://ispc.github.io), version 1.14.1 or later. Please
    obtain a release of ISPC from the [ISPC downloads
    page](https://ispc.github.io/downloads.html).

  - Open VKL depends on the Intel RenderKit common library, rkcommon.
    rkcommon is available at the [rkcommon GitHub
    repository](https://github.com/ospray/rkcommon).

  - Open VKL depends on Embree, which is available at the [Embree GitHub
    repository](https://github.com/embree/embree).

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
