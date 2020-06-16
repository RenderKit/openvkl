// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VdbGrid.h"
#include "VdbIterator.ih"
#include "common/export_util.h"
#include "math/box_utility.ih"
#include "math/math.ih"

export void EXPORT_UNIQUE(VdbIterator_export,
                          uniform box1f &dummy_box1f,
                          const DdaSegmentState *uniform ddaSegmentState,
                          const DdaLevelState *uniform ddaLevelState,
                          const varying VdbIterator *uniform it)
{
}

export uniform int EXPORT_UNIQUE(VdbIterator_sizeOf)
{
  return sizeof(varying VdbIterator);
}

export void EXPORT_UNIQUE(VdbIterator_Initialize,
                          const int *uniform imask,
                          void *uniform _self,
                          const void *uniform _grid,
                          void *uniform _originObject,
                          void *uniform _directionObject,
                          void *uniform _tRangeWorld,
                          void *uniform _valueSelector)
{
  varying VdbIterator *uniform self = (varying VdbIterator * uniform) _self;

  if (!imask[programIndex]) {
    return;
  }

  const uniform VdbGrid *uniform grid = (const uniform VdbGrid *uniform)_grid;
  const vec3f origin    = *((const varying vec3f *uniform)_originObject);
  const vec3f direction = *((const varying vec3f *uniform)_directionObject);
  const box1f tRange    = *((const varying box1f *uniform)_tRangeWorld);

  // Always initialize the root level iterator!
  self->grid          = grid;
  self->valueSelector = (uniform ValueSelector * uniform) _valueSelector;
  self->numLevels = clamp(grid->maxIteratorDepth, 0, VDB_ITERATOR_MAX_LEVELS);

  // Transform the ray to index space where leaf level voxels have
  // size (1,1,1) and the root is at (0,0,0).
  const vec3f rootOffset =
      make_vec3f(grid->rootOrigin.x, grid->rootOrigin.y, grid->rootOrigin.z);
  ddaInitRay(xfmPoint(grid->objectToIndex, origin) - rootOffset,
             xfmVector(grid->objectToIndex, direction),
             tRange,
             self->ddaRayState);

  // This is an estimate of how far apart voxels are along the ray in object
  // space. We are basically measuring here how much the volume is scaled along
  // the ray, and voxels in index space have size 1.
  self->nominalDeltaT = length(direction) / length(self->ddaRayState.rayDir);

  for (uniform size_t i = 0; i < self->numLevels; ++i) {
    ddaInitLevel(self->ddaRayState,
                 vklVdbLevelTotalLogRes(i + 1),
                 vklVdbLevelTotalLogRes(i),
                 self->ddaLevelState[i]);
  }

  // Initialize the root node segment so that we are ready to go.
  const varying vec3i rootNodeOffset = make_vec3i(0, 0, 0);
  self->currentLevel                 = 0;
  self->nodeIndex[0]                 = 0;
  ddaInitSegment(self->ddaRayState,
                 self->ddaLevelState[0],
                 rootNodeOffset,
                 self->ddaSegmentState[0]);
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

  varying VdbIterator *uniform self  = (varying VdbIterator * uniform) _self;
  varying Interval *uniform interval = (varying Interval * uniform) _interval;
  varying int *uniform result        = (varying int *uniform)_result;
  const VdbGrid *uniform grid        = self->grid;

  interval->valueRange.lower             = inf;
  interval->valueRange.upper             = neg_inf;
  const varying DdaRayState &ddaRayState = self->ddaRayState;

  *result   = false;
  bool done = false;
  int iter  = 0;
  while (!done) {
    foreach_unique(currentLevel in self->currentLevel)
    {
      ++iter;
      assert(currentLevel < VDB_ITERATOR_MAX_LEVELS);
      varying DdaLevelState &ddaLevelState = self->ddaLevelState[currentLevel];
      varying DdaSegmentState &ddaSegmentState =
          self->ddaSegmentState[currentLevel];

      if (!ddaStateHasExited(ddaSegmentState)) {
        if (ddaStateInBounds(ddaSegmentState)) {
          const uint64 vidx = vklVdbDomainOffsetToLinear(currentLevel,
                                                         ddaSegmentState.idx.x,
                                                         ddaSegmentState.idx.y,
                                                         ddaSegmentState.idx.z);
          assert(vidx < vklVdbLevelNumVoxels(currentLevel));

          const varying uint64 nodeVoxelOffset =
              self->nodeIndex[currentLevel] *
              vklVdbLevelNumVoxels(currentLevel);
          const varying uint64 voxelOffset = nodeVoxelOffset + vidx;
          assert(voxelOffset < ((varying uint64)1) << 32);

          const varying uint32 vo32 = ((varying uint32)voxelOffset);
          const varying uint64 voxelValue =
              grid->levels[currentLevel].voxels[vo32];
          const varying range1f valueRange =
              grid->levels[currentLevel].valueRange[vo32];

          if ((self->valueSelector &&
               !overlaps1f(self->valueSelector->rangesMinMax, valueRange)) ||
              vklVdbVoxelIsEmpty(voxelValue)) {
            ddaStep(self->ddaRayState,
                    self->ddaLevelState[currentLevel],
                    self->ddaSegmentState[currentLevel]);
          } else {
            // We count inner nodes that we cannot expand as leaves.
            const bool isTile = vklVdbVoxelIsTile(voxelValue);
            const bool isLeaf = vklVdbVoxelIsLeafPtr(voxelValue) ||
                                (vklVdbVoxelIsChildPtr(voxelValue) &&
                                 (currentLevel + 1) >= self->numLevels);
            const bool isInner = !isLeaf && vklVdbVoxelIsChildPtr(voxelValue);

            if (isTile || isLeaf) {
              interval->valueRange    = valueRange;
              interval->tRange.lower  = ddaSegmentState.t;
              interval->tRange.upper  = reduce_min(ddaSegmentState.tNext);
              interval->nominalDeltaT = self->nominalDeltaT;

              *result = true;
              done    = true;

              ddaStep(self->ddaRayState,
                      self->ddaLevelState[currentLevel],
                      self->ddaSegmentState[currentLevel]);
            }

            else {
              assert(isInner);
              ++self->currentLevel;
              self->nodeIndex[currentLevel + 1] =
                  vklVdbVoxelChildGetIndex(voxelValue);
              ddaInitSegment(self->ddaRayState,
                             self->ddaLevelState[currentLevel + 1],
                             ddaSegmentState.idx,
                             self->ddaSegmentState[currentLevel + 1]);
              // Do not step in this case - ddaInit initializes to the first
              // valid interval already.
            }
          }
        } else  // ddaStateInBounds
        {
          // This happens mostly at the end of iteration: incremental
          // computation of t may result in values slightly less than tMax, so
          // we end up inside the t range, but outside our domain.
          ddaStep(self->ddaRayState,
                  self->ddaLevelState[currentLevel],
                  self->ddaSegmentState[currentLevel]);
        }
      } else  // !ddaStateHasExited -- we are out of bounds on the current
              // level.
      {
        if (currentLevel ==
            0)  // There is no parent level. We have left the volume.
        {
          done = true;
        } else  // There is a parent level. Go up.
        {
          --self->currentLevel;
          ddaStep(self->ddaRayState,
                  self->ddaLevelState[currentLevel - 1],
                  self->ddaSegmentState[currentLevel - 1]);
        }
      }
    }  // foreach_unique(currentLevel in self->currentLevel)
  }

  assert(done);
}
