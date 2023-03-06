// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"

#include "../../common/ValueRanges.ih"
#include "VdbGrid.h"
#include "VdbIterator.ih"
#include "VdbQueryVoxelDense.ih"
#include "VdbSamplerShared.h"
#include "common/export_util.h"
#include "math/box_utility.ih"

// Generated headers.
#include "openvkl_vdb/HDDA_0.ih"
#include "openvkl_vdb/VdbQueryVoxel_0.ih"

// Ignore warning about exporting uniform-pointer-to-varying, as this is in
// fact legal.
#pragma ignore warning(all)
export void EXPORT_UNIQUE(VdbIterator_export,
                          uniform box1f &dummy_box1f,
                          const varying VdbIterator *uniform it)
{
}

void VdbIterator_iterateIntervalInternal(
    const int *uniform imask,
    void *uniform _self,
    void *uniform _interval,
    const uniform ValueRanges &valueRanges,
    const uniform bool /*elementaryCellIteration*/,
    uniform int *uniform _result);

export void EXPORT_UNIQUE(VdbIterator_Initialize,
                          const int *uniform imask,
                          void *uniform _self,
                          const void *uniform _context,
                          void *uniform _originObject,
                          void *uniform _directionObject,
                          void *uniform _tRangeWorld)
{
  if (!imask[programIndex]) {
    return;
  }

  varying VdbIterator *uniform self = (varying VdbIterator * uniform) _self;

  self->super.super.context = (const IntervalIteratorContext *uniform)_context;
  self->super.iterate       = VdbIterator_iterateIntervalInternal;
  self->super.elementaryCellIterationSupported = false;

  const uniform VdbSamplerShared *uniform sampler =
      (const uniform VdbSamplerShared *uniform)(
          self->super.super.context->super.sampler);
  const uniform VdbGrid *uniform grid = sampler->grid;

  self->grid = grid;

  const vec3f origin    = *((const varying vec3f *uniform)_originObject);
  const vec3f direction = *((const varying vec3f *uniform)_directionObject);
  box1f tRange          = *((const varying box1f *uniform)_tRangeWorld);

  // Transform the ray to index space where leaf level voxels have
  // size (1,1,1) and the root is at (0,0,0).
  const vec3f rootOffset =
      make_vec3f(grid->rootOrigin.x, grid->rootOrigin.y, grid->rootOrigin.z);
  const vec3f rayOrg = xfmPoint(grid->objectToIndex, origin) - rootOffset;
  const vec3f rayDir = xfmVector(grid->objectToIndex, direction);

  // This is an estimate of how far apart voxels are along the ray in object
  // space. We are basically measuring here how much the volume is scaled along
  // the ray, and voxels in index space have size 1. Note that this also
  // accounts for potentially different object-space voxel spacings per
  // dimension.
  self->nominalDeltaT = reduce_min(1.f * rcp_safe(absf(rayDir)));

  // Clip on active region.
  const box1f tRangeIntersected =
      intersectBox(rayOrg, rayDir, grid->domainBoundingBox, tRange);

  if (isempty1f(tRangeIntersected)) {
    self->tMax  = neg_inf;
    self->dda.t = 0;
  } else {
    self->tMax = tRangeIntersected.upper;
    ddaInit(rayOrg,
            rayDir,
            tRangeIntersected.lower,
            self->super.super.context->super.maxIteratorDepth,
            self->dda);
  }
}

inline void VdbIterator_iterateIntervalInternal(
    const int *uniform imask,
    void *uniform _self,
    void *uniform _interval,
    const uniform ValueRanges &inputValueRanges,
    const uniform bool /*elementaryCellIteration*/,
    uniform int *uniform _result)
{
  if (!imask[programIndex]) {
    return;
  }

  varying int *uniform result = (varying int *uniform)_result;
  *result                     = false;

  varying VdbIterator *uniform self = (varying VdbIterator * uniform) _self;
  if (!(self->dda.t <= self->tMax)) {
    return;
  }

  VdbVoxelDescriptor voxel;
  while (self->dda.t <= self->tMax) {
    uint32 exitDepth = 0;
    if (self->grid->dense) {
      VdbIterator_queryVoxel_dense(
          self->grid,
          self->dda.idx[DDA_STATE_X_OFFSET(self->dda.level)],
          self->dda.idx[DDA_STATE_Y_OFFSET(self->dda.level)],
          self->dda.idx[DDA_STATE_Z_OFFSET(self->dda.level)],
          self->dda.level,
          self->super.super.context->super.attributeIndex,
          inputValueRanges,
          voxel);
    } else {
      VdbIterator_queryVoxel(self->grid,
                             self->dda.idx[DDA_STATE_X_OFFSET(self->dda.level)],
                             self->dda.idx[DDA_STATE_Y_OFFSET(self->dda.level)],
                             self->dda.idx[DDA_STATE_Z_OFFSET(self->dda.level)],
                             self->dda.level,
                             self->super.super.context->super.attributeIndex,
                             inputValueRanges,
                             voxel);
    }

    if (!voxel.isEmpty)
      break;

    hddaStep(self->grid, voxel, self->dda);
  }

  if (self->dda.t < self->tMax) {
    varying Interval *uniform interval = (varying Interval * uniform) _interval;
    interval->valueRange               = voxel.valueRange;
    interval->tRange.lower             = self->dda.t;

    // Move on to the next voxel to fulfill invariant.
    hddaStep(self->grid, voxel, self->dda);

    interval->tRange.upper  = min(self->dda.t, self->tMax);
    interval->nominalDeltaT = self->nominalDeltaT;

    *result = true;
  }
}

export void EXPORT_UNIQUE(VdbIterator_iterateInterval,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _interval,
                          uniform int *uniform _result)
{
  if (!imask[programIndex]) {
    return;
  }

  varying VdbIterator *uniform self = (varying VdbIterator * uniform) _self;

  VdbIterator_iterateIntervalInternal(
      imask,
      _self,
      _interval,
      self->super.super.context->super.valueRanges,
      false,
      _result);
}
