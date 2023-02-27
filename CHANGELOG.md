Version History
---------------

### Open VKL 1.3.2

-   Move to and require latest versions of RenderKit dependencies: Embree v4.0.0
    and rkcommon v1.11.0
-   ARM support: expose ISPC neon-i32x8 target via OPENVKL_ISA_NEON2X CMake
    option
-   Superbuild updates to latest versions of dependencies

### Open VKL 1.3.1

-   Superbuild updates to latest versions of dependencies
-   Note that the update to zlib v1.2.13 remedies CVE-2022-37434

### Open VKL 1.3.0

-   Added AVX512 8-wide CPU device mode, enabled via the
    `OPENVKL_ISA_AVX512SKX_8_WIDE` CMake option
-   VDB volumes: added support for packed / contiguous data layouts for
    temporally constant volumes, which can provide improved performance
    (`nodesPackedDense`, `nodesPackedTile` parameters)
-   VDB utility library: added `repackNodes` flag to toggle usage of packed data
    layouts
-   Particle volumes: general memory efficiency and performance improvements
-   Superbuild updates to latest versions of dependencies
-   Minimum ISPC version is now v1.18.0

### Open VKL 1.2.0

-   Added `vklSetParam()` API function which can set parameters of any supported
    type
-   Structured regular volumes:
    -   Added support for cell-centered data via the `cellCentered` parameter;
        vertex-centered remains the default
    -   Added support for more general transformations via the `indexToObject`
        parameter
    -   Added `indexOrigin` parameter which applies an index-space vec3i
        translation
-   VDB volumes:
    -   Added `indexClippingBounds` parameter, which can restrict the active
        voxel bounding box
    -   The `indexToObject` parameter can now be provided as a `VKL_AFFINE3F`
    -   Corrected bounding box computations in `InnerNode` observer
-   Particle volumes:
    -   Now ignoring particles with zero radius
-   VDB utility library: added `commit` flag (default true) to volume creation
    methods, allowing apps to set additional parameters before first commit
-   Examples:
    -   Added new set of minimal examples, which step through creation of basic
        volume and isosurface renderers
    -   Exposing `intervalResolutionHint` parameter in `vklExamples` application
-   Superbuild updates to latest versions of dependencies

### Open VKL 1.1.0

-   vklExamples improvements: asynchronous rendering, multiple viewports,
    docking, and more
-   Fixed bug in `openvkl_utility_vdb` which could lead to crashes when creating
    VDB volumes with temporally constant tiles
-   Superbuild updates to latest versions of dependencies
-   Minimum rkcommon version is now 1.8.0

### Open VKL 1.0.1

-   Fixed issue in `structuredRegular` and `vdb` interval iterators that could
    lead to erroneous initial intervals for certain ray inputs
-   Fixed handling of `intervalResolutionHint` interval iterator context
    parameter for `amr`, `particle`, and `unstructured` volumes with small
    numbers of cells / primitives

### Open VKL 1.0.0

-   The version 1.0 release marks long term API stability (until v2.0)
-   Open VKL can now be built for ARM CPUs that support Neon
-   Iterator API updates:
    -   Introducing interval and hit iterator contexts, which hold
        iterator-specific configuration (eliminates value selector objects)
    -   Interval and hit iteration is now supported on any volume attribute
    -   Interval iterators now include a `time` parameter
    -   Interval iterators now support the `intervalResolutionHint` parameter,
        replacing `maxIteratorDepth` and `elementaryCellIteration`
-   Supporting configurable background values; default is now
    `VKL_BACKGROUND_UNDEFINED` (NaN) for all volume types
-   `vklGetValueRange()` now supports all volume attributes
-   Added ISPC-side API bindings for `vklGetNumAttributes()` and
    `vklGetValueRange()`
-   Structured regular volumes:
    -   Added support for tricubic filtering
    -   More accurate gradient computations respecting filter mode
    -   Hit iteration robustness improvements
-   VDB volumes:
    -   Interval and hit iteration robustness improvements
    -   Corrected interval iterator `nominalDeltaT` computation for
        non-normalized ray directions and non-uniform object-space grid spacings
    -   Fixed bug which could cause incorrect value range computations for
        temporally varying volumes
-   vklExamples additions demonstrating:
    -   Multi-attribute interval / hit iteration
    -   Configurable background values
    -   Temporally varying volumes
-   Superbuild updates to latest versions of dependencies
-   Now requiring minimum versions:
    -   Embree 3.13.1
    -   rkcommon 1.7.0
    -   ISPC 1.16.0

### Open VKL 0.13.0

-   Driver (now device) API changes:
    -   Renamed `VKLDriver` to `VKLDevice` and updated associated device setup
        APIs
    -   Use of multiple concurrent devices is now supported; therefore
        `vklNewVolume()` and `vklNewData()` now require a device handle
    -   Renamed the `ispc_device` module and `ispc` device to `cpu_device` and
        `cpu`, respectively
    -   The `OPENVKL_CPU_DEVICE_DEFAULT_WIDTH` environment variable can now be
        used to change the `cpu` device's default SIMD width at run time
-   Added new `VKLTemporalFormat` enum used for temporally varying volume
    parameterization
-   VDB volumes:
    -   Support for temporally structured and temporally unstructured (TUV)
        attribute data, which can be used for motion blurred rendering
    -   Supporting tricubic filtering via `VKL_FILTER_TRICUBIC` filter type
    -   Added support for half precision float-point (FP16) attribute data via
        `VKL_HALF` data type
    -   Added a new `InnerNode` observer and associated utility functions which
        allows applications to introspect inner nodes of the internal tree
        structure, including bounding boxes and value ranges
    -   Renamed `VKL_FORMAT_CONSTANT_ZYX` to `VKL_FORMAT_DENSE_ZYX`
-   Structured regular and spherical volumes:
    -   Added support for half precision float-point (FP16) attribute data via
        `VKL_HALF` data type
-   Unstructured volumes:
    -   Added support for elementary cell iteration via the
        `elementaryCellIteration` parameter
    -   Robustness improvements for hit iteration
-   AMR volumes:
    -   Improved interval iterator implementation, resolving issues with
        returned interval `nominalDeltaT` values
    -   Interval iterators now support `maxIteratorDepth` parameter
-   Interval and hit iteration performance improvements when multiple values
    ranges / values are selected
-   Added new temporal compression utilities which applications can use for
    processing temporally unstructured attribute data
-   vklExamples additions demonstrating:
    -   Motion blurred rendering on temporally structured and temporally
        unstructured `vdb` volumes
    -   Tricubic filtering on `vdb` volumes
    -   Half-precision floating-point (FP16) support for `structuredRegular`,
        `structuredSpherical`, and `vdb` volumes
    -   Elementary cell interval iteration on `unstructured` volumes
    -   Use of the `InnerNode` observer on `vdb` volumes
-   Superbuild updates to:
    -   Embree 3.13.0
    -   rkcommon 1.6.1
-   Minimum rkcommon version is now 1.6.1

### Open VKL 0.12.1

-   Fixed bug in VDB volume interval iterator implementation which could lead to
    missed intervals or incorrect value ranges in returned intervals

### Open VKL 0.12.0

-   Added support for temporally varying volumes with associated API changes for
    sampling, gradients, and hit iteration. This feature can be used to enable
    motion blurred rendering
-   Structured regular volumes:
    -   Support for temporally structured and temporally unstructured (TUV)
        input data
    -   Improved `nominalDeltaT` for interval iteration
    -   Interval iterator robustness improvements for axis-aligned rays
    -   Sampling performance improvements
-   VDB volumes:
    -   Multi-attribute support (including three-component float grids)
    -   Interval iterator robustness improvements for axis-aligned rays
    -   Performance improvements for scalar sampling
    -   Now restricting volumes to exactly four levels
    -   Allowing leaf nodes on the lowest level only
-   Unstructured volumes:
    -   Improved `nominalDeltaT` for interval iteration
-   `vdb_util` updates:
    -   Support for loading multi-attribute .vdb files (`float` and `Vec3s`
        grids)
    -   Fix order of rotation matrix coefficients loaded from .vdb files
-   vklExamples additions demonstrating:
    -   Motion blurred rendering on temporally structured and temporally
        unstructured volumes (`structuredRegular` only)
    -   Support for `vdb` multi-attribute volumes
    -   Hit iterator time support
-   Superbuild updates to:
    -   Embree 3.12.2
    -   rkcommon 1.6.0
    -   ISPC 1.15.0
    -   OpenVDB 8.0.0
-   Minimum rkcommon version is now 1.6.0

### Open VKL 0.11.0

-   Introduced API support for multi-attribute volumes, including APIs for
    sampling multiple attributes simultaneously
    -   Initially only `structuredRegular` and `structuredSpherical` volume
        types support multi-attribute data
-   Iterator APIs now work on sampler objects rather than volumes, supporting
    finer-grained configurability
-   Observers can now be created for both volume and sampler objects
    -   `LeafNodeAccess` observers must now be created on sampler objects
-   Log and error callbacks now support a user pointer
-   `vdb` volume interval iterators:
    -   Added support for elementary cell iteration when `maxIteratorDepth` is
        set to `VKL_VDB_NUM_LEVELS`-1
    -   Up to 2x faster iteration
-   `unstructured` and `particle` volume interval iterators:
    -   Improved interior empty space skipping behavior
    -   Added support for configurable iterator depth via the `maxIteratorDepth`
        parameter
-   Added support for filter modes in `structuredRegular` and
    `structuredSpherical` volumes
-   `amr` volumes now support `method` parameter on sampler objects
-   Added new `interval_iterator_debug` renderer in `vklExamples` to visualize
    interval iteration behavior
-   Hit iterator accuracy improvements for `unstructured` volumes
-   Fixed bugs in `amr` and `vdb` volume bounding box computations
-   Fixed bug in `unstructured` volume gradient computations near empty regions
-   Minimum ISPC version is now v1.14.1

### Open VKL 0.10.0 (alpha)

-   Added new `particle` volume type supporting Gaussian radial basis functions
-   Introduced `VKLSampler` objects allowing configuration of sampling and
    gradient behavior
-   Added stream-wide sampling and gradient APIs
-   Introduced a new way to allocate iterators, giving the user more freedom in
    choosing allocation schemes and reducing iterator size
-   Added support for strided data arrays
-   Added gradient implementations for `amr` and `vdb` volumes
-   Hit iterator accuracy improvements for `amr`, `structuredSpherical`,
    `unstructured`, and `vdb` volumes
-   Up to 4x performance improvement for `structuredRegular` and
    `structuredSpherical` sampling for volumes in the 1-2GB range
-   Up to 2x performance improvement for `structuredRegular` interval iteration
-   Improved commit speed for `unstructured` volumes
-   Improved value range computation in `vdb` volumes
-   Improved isosurface shading in `vklExamples`
-   Improved parameter validation across all volume types
-   Aligned `VKLHit[4,8,16]` and `VKLInterval[4,8,16]` structs
-   Added hit epsilon to `VKLHit[4,8,16]`
-   Updated parameter names for `vdb` volumes
-   Renamed `VKLVdbLeafFormat` to `VKLFormat`
-   Fixed incorrect use of system-installed CMake in superbuild while building
    dependencies
-   Fixed various memory leaks
-   Fixed crashes which could occur in `VdbVolume::cleanup()` and
    `vklShutdown()`
-   Moved from ospcommon to rkcommon v1.4.1

### Open VKL 0.9.0 (alpha)

-   Added support for VDB sparse structured volumes (`"vdb"` volume type)
-   Added `vdb_util` library to simplify instantiation of VDB volumes, and
    support loading of .vdb files using OpenVDB
-   Added `VKLObserver` and associated APIs, which may used by volume types to
    pass information back to the application
    - A `LeafNodeAccess` observer is provided for VDB volumes to support
      on-demand loading of leaf nodes
-   Structured regular volumes:
    -   Up to 6x performance improvement for scalar iterator initialization
    -   Up to 2x performance improvement for scalar iterator iteration
-   General improvements to the CMake Superbuild for building Open VKL and all
    associated dependencies
-   Allowing instantiation of ISPC driver for any supported SIMD width (in
    addition to the default automatically selected width)
-   Volume type names are now camelCase (legacy snake_case type names are
    deprecated), impacting `structuredRegular` and `structuredSpherical` volumes
-   Enabling `flushDenormals` driver mode by default
-   Aligning public `vkl_vvec3f[4,8,16]` and `vkl_vrange1f[4,8,16]` types
-   Added `VKL_LOG_NONE` log level
-   Fixed bug in `vklExamples` which could lead to improper rendering on macOS
    Catalina
-   Fixed bug in unstructured volume interval iterator which could lead to
    errors with some combinations of lane masks
-   Now providing binary releases for Linux, macOS, and Windows

### Open VKL 0.8.0 (alpha)

-   Added support for structured volumes on spherical grids
    (`"structured_spherical"` volume type)
-   Structured regular volumes:
    -   Up to 8x performance improvement for scalar (single-wide) sampling
    -   Fixed hit iterator bug which could lead to isosurfacing artifacts
    -   Renamed `voxelData` parameter to `data`
-   Unstructured volumes:
    -   Up to 4x performance improvement for scalar (single-wide) sampling
    -   Improved interval iterator implementation for more efficient space
        skipping and tighter value bounds on returned intervals
    -   Now using Embree for BVH builds for faster build times / volume commits
    -   Renamed `vertex.value` and `cell.value` parameters to `vertex.data` and
        `cell.data`, respectively
-   AMR volumes:
    -   renamed `block.cellWidth` parameter to `cellWidth`, and clarified API
        documentation
-   Added `vklGetValueRange()` API for querying volume value ranges
-   Added new driver parameters, APIs, and environment variables allowing user
    control of log levels, log / error output redirection, number of threads,
    and other options
-   `vklIterateHit[4,8,16]()` and `vklIterateInterval[4,8,16]()` calls now only
    populate hit / interval data for active lanes
-   Changed `VKLDataType` enum values for better forward compatibility
-   ISPC-side hit and interval iterator objects must now be declared `varying`
-   More flexible ISA build configuration through `OPENVKL_MAX_ISA` and
    `OPENVKL_ISA_*` CMake build options
-   Minimum ospcommon version is now 1.1.0

### Open VKL 0.7.0 (alpha)

-   Initial public alpha release, with support for structured, unstructured, and
    AMR volumes.
