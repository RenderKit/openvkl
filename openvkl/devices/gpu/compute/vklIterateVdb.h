// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/math/math.ih"

#include "../../cpu/volume/vdb/VdbGrid.h"

namespace ispc {

  // facilitates direct reuse of the below includes
  using openvkl::cpu_device::vklVdbVoxelChildGetIndex;
  using openvkl::cpu_device::vklVdbVoxelIsChildPtr;
  using openvkl::cpu_device::vklVdbVoxelIsEmpty;
  using openvkl::cpu_device::vklVdbVoxelIsLeafPtr;

#include "../../cpu/volume/vdb/Dda.ih"
#include "openvkl_vdb/VdbQueryVoxel_0.ih"

#include "openvkl_vdb/HDDA_0.ih"

  struct VdbIntervalIterator
  {
    IntervalIteratorShared intervalIteratorShared;

    const VdbGrid *grid;

    float tMax;
    float nominalDeltaT;

    DdaState dda;
  };

  inline void VdbIntervalIterator_Init(VdbIntervalIterator *self,
                                       const IntervalIteratorContext *context,
                                       const vec3f *origin,
                                       const vec3f *direction,
                                       const box1f *tRange)
  {
    self->intervalIteratorShared.context = context;

    const VdbSamplerShared *sampler =
        (const VdbSamplerShared *)(self->intervalIteratorShared.context->super
                                       .sampler);
    const VdbGrid *grid = sampler->grid;

    self->grid = grid;

    // Transform the ray to index space where leaf level voxels have
    // size (1,1,1) and the root is at (0,0,0).
    const vec3f rootOffset =
        make_vec3f(grid->rootOrigin.x, grid->rootOrigin.y, grid->rootOrigin.z);
    const vec3f rayOrg =
        openvkl::cpu_device::xfmPoint(grid->objectToIndex, *origin) -
        rootOffset;
    const vec3f rayDir =
        openvkl::cpu_device::xfmVector(grid->objectToIndex, *direction);

    // This is an estimate of how far apart voxels are along the ray in object
    // space. We are basically measuring here how much the volume is scaled
    // along the ray, and voxels in index space have size 1. Note that this also
    // accounts for potentially different object-space voxel spacings per
    // dimension.
    self->nominalDeltaT = reduce_min(1.f * rcp_safe(absf(rayDir)));

    // Clip on active region.
    const box1f tRangeIntersected =
        intersectBox(rayOrg, rayDir, grid->domainBoundingBox, *tRange);

    if (isempty1f(tRangeIntersected)) {
      self->tMax  = neg_inf;
      self->dda.t = 0;
    } else {
      self->tMax = tRangeIntersected.upper;
      ddaInit(rayOrg,
              rayDir,
              tRangeIntersected.lower,
              self->intervalIteratorShared.context->super.maxIteratorDepth,
              self->dda);
    }
  }

  inline void VdbIntervalIterator_iterate(VdbIntervalIterator *self,
                                          Interval *interval,
                                          int *result)
  {
    *result = false;

    if (!(self->dda.t <= self->tMax)) {
      return;
    }

    VdbVoxelDescriptor voxel;
    while (self->dda.t <= self->tMax) {
      uint32 exitDepth = 0;

      VdbIterator_queryVoxel(
          self->grid,
          self->dda.idx[DDA_STATE_X_OFFSET(self->dda.level)],
          self->dda.idx[DDA_STATE_Y_OFFSET(self->dda.level)],
          self->dda.idx[DDA_STATE_Z_OFFSET(self->dda.level)],
          self->dda.level,
          self->intervalIteratorShared.context->super.attributeIndex,
          self->intervalIteratorShared.context->super.valueRanges,
          voxel);

      if (!voxel.isEmpty)
        break;

      hddaStep(self->grid, voxel, self->dda);
    }

    if (self->dda.t < self->tMax) {
      interval->valueRange   = voxel.valueRange;
      interval->tRange.lower = self->dda.t;

      // Move on to the next voxel to fulfill invariant.
      hddaStep(self->grid, voxel, self->dda);

      interval->tRange.upper  = min(self->dda.t, self->tMax);
      interval->nominalDeltaT = self->nominalDeltaT;

      *result = true;
    }
  }

}  // namespace ispc
