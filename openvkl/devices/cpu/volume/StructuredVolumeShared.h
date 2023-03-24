// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VolumeShared.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  struct GridAccelerator;

#ifdef __cplusplus

  typedef void *ComputeSampleInnerVaryingFunc;

  typedef void *ComputeSampleInnerUniformFunc;

  typedef void *ComputeGradientVaryingFunc;

  typedef void *ComputeVoxelRangeFunc;

#else

typedef varying float (*uniform ComputeSampleInnerVaryingFunc)(
    const SharedStructuredVolume *uniform self,
    const varying vec3f &clampedLocalCoordinates,
    const uniform VKLFilter filter,
    const uniform uint32 attributeIndex,
    const varying float &times);

typedef uniform float (*uniform ComputeSampleInnerUniformFunc)(
    const SharedStructuredVolume *uniform self,
    const uniform vec3f &clampedLocalCoordinates,
    const uniform VKLFilter filter,
    const uniform uint32 attributeIndex,
    const uniform float &times);

typedef varying vec3f (*uniform ComputeGradientVaryingFunc)(
    const SharedStructuredVolume *uniform _self,
    const varying vec3f &objectCoordinates,
    const uniform VKLFilter filter,
    const uniform uint32 attributeIndex,
    const varying float &time);

typedef varying range1f (*uniform ComputeVoxelRangeFunc)(
    const SharedStructuredVolume *uniform self,
    const varying vec3i &localCoordinates,
    const uniform uint32 attributeIndex);

#endif  // __cplusplus

  enum SharedStructuredVolumeGridType
  {
    structured_regular,
    structured_spherical
  };

  struct SharedStructuredVolume
  {
    VolumeShared super;

    VKL_INTEROP_UNIFORM uint32 numAttributes;
    Data1D *VKL_INTEROP_UNIFORM attributesData;

    VKL_INTEROP_UNIFORM uint32 temporallyStructuredNumTimesteps;
    VKL_INTEROP_UNIFORM Data1D temporallyUnstructuredIndices;
    VKL_INTEROP_UNIFORM Data1D temporallyUnstructuredTimes;

    VKL_INTEROP_UNIFORM vec3i dimensions;

    VKL_INTEROP_UNIFORM SharedStructuredVolumeGridType gridType;
    VKL_INTEROP_UNIFORM vec3f gridOrigin;
    VKL_INTEROP_UNIFORM vec3f gridSpacing;

    VKL_INTEROP_UNIFORM box3f boundingBox;

    // value range for first attribute only, to support interval iterators
    VKL_INTEROP_UNIFORM box1f valueRange0;

    VKL_INTEROP_UNIFORM vec3f localCoordinatesUpperBound;

    GridAccelerator *VKL_INTEROP_UNIFORM accelerator;

    // offsets, in voxels, for one step in x,y,z direction; ONLY valid if
    // bytesPerSlice < 2G.
    VKL_INTEROP_UNIFORM uint32 voxelOfs_dx, voxelOfs_dy, voxelOfs_dz;

    // This is only used by the legacy sample wrappers - remove once those
    // have disappeared.
    VKLFilter filter;

    // varying functions
    ComputeVoxelRangeFunc *VKL_INTEROP_UNIFORM computeVoxelRange;

    ComputeSampleInnerVaryingFunc *VKL_INTEROP_UNIFORM
        computeSamplesInner_varying;
    ComputeGradientVaryingFunc computeGradient_varying;

    // uniform functions
    ComputeSampleInnerUniformFunc *VKL_INTEROP_UNIFORM
        computeSamplesInner_uniform;
  };

#ifdef __cplusplus
}
#endif  // __cplusplus
