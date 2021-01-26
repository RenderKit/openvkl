Version History
---------------

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
