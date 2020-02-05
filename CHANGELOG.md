Version History
---------------

### Open VKL 0.9.0 (alpha)

-   Added basic support for VDB volumes (`"vdb"` volume type).
-   Added `VLKObserver`, which may be provided by volume types
    to pass information back to the application.

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
