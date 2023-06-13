// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rkcommon/math/box.ih>
#include <rkcommon/math/math.ih>

#include "../../cpu/common/ValueRangesShared.h"
#include "../../cpu/iterator/IteratorContextShared.h"
#include "../../cpu/iterator/IteratorShared.h"
#include "../../cpu/math/box_utility.ih"
#include "../../cpu/volume/GridAccelerator.h"
#include "../../cpu/volume/GridAcceleratorShared.h"
#include "../../cpu/volume/StructuredVolumeShared.h"
#include "../common/Data.ih"

#include "common.h"
#include "iterator_common.h"

namespace ispc {

  // from GridAcceleratorIterator.ih
  struct GridAcceleratorIteratorIntervalState
  {
    vec3i currentCellIndex;
    float nominalDeltaT;  // constant for all intervals
  };

  struct GridAcceleratorIterator
  {
    IntervalIteratorShared super;

    vec3f origin;
    vec3f direction;
    box1f tRange;
    float time;

    // common state
    box1f boundingBoxTRange;

    // interval iterator state
    GridAcceleratorIteratorIntervalState intervalState;
  };

  // from GridAcceleratorIterator.ispc
  inline void GridAcceleratorIteratorU_Init(
      GridAcceleratorIterator *self,
      const IntervalIteratorContext *context,
      const vec3f *origin,
      const vec3f *direction,
      const box1f *tRange,
      const float *time)
  {
    self->super.context = context;
    self->origin        = *origin;
    self->direction     = *direction;
    self->tRange        = *tRange;
    self->time          = *time;

    const SharedStructuredVolume *volume =
        (const SharedStructuredVolume *)
            self->super.context->super.sampler->volume;

    self->boundingBoxTRange = intersectBox(
        self->origin, self->direction, volume->boundingBox, self->tRange);

    self->intervalState.currentCellIndex = vec3i{-1, -1, -1};

    self->intervalState.nominalDeltaT =
        reduce_min(volume->gridSpacing *
                   rcp_safe(absf(self->direction))); /* in ray space */
  }

  // from SharedStructuredVolume.ih
  inline void transformLocalToObject_uniform_structured_regular(
      const SharedStructuredVolume *self,
      const vec3f &localCoordinates,
      vec3f &objectCoordinates)
  {
    objectCoordinates =
        vec3f{self->gridOrigin.x + localCoordinates.x * self->gridSpacing.x,
              self->gridOrigin.y + localCoordinates.y * self->gridSpacing.y,
              self->gridOrigin.z + localCoordinates.z * self->gridSpacing.z};
  }

  // from GridAccelerator.ispc
  inline uint32 GridAccelerator_getCellIndex1D(GridAccelerator *accelerator,
                                               const vec3i &cellIndex)
  {
    const vec3i brickIndex = {cellIndex.x >> BRICK_WIDTH_BITCOUNT,
                              cellIndex.y >> BRICK_WIDTH_BITCOUNT,
                              cellIndex.z >> BRICK_WIDTH_BITCOUNT};

    const uint32 brickAddress =
        brickIndex.x + accelerator->bricksPerDimension.x *
                           (brickIndex.y + accelerator->bricksPerDimension.y *
                                               (uint32)brickIndex.z);

    const vec3i cellOffset = bitwise_AND(cellIndex, BRICK_WIDTH - 1);

    return brickAddress << (3 * BRICK_WIDTH_BITCOUNT) |
           cellOffset.z << (2 * BRICK_WIDTH_BITCOUNT) |
           cellOffset.y << (BRICK_WIDTH_BITCOUNT) | cellOffset.x;
  }

  inline void GridAccelerator_getCellValueRange(GridAccelerator *accelerator,
                                                const vec3i &cellIndex,
                                                uint32 attributeIndex,
                                                box1f &valueRange)
  {
    const uint32 cellIndex1D =
        GridAccelerator_getCellIndex1D(accelerator, cellIndex);
    auto _valueRange =
        accelerator->cellValueRanges[attributeIndex * accelerator->cellCount +
                                     cellIndex1D];
    valueRange = {_valueRange.lower, _valueRange.upper};
  }

  inline box3f GridAccelerator_getCellBounds(const GridAccelerator *accelerator,
                                             const vec3i &index)
  {
    SharedStructuredVolume *volume = accelerator->volume;

    /* coordinates of the lower corner of the cell in object coordinates */
    vec3f lower;
    transformLocalToObject_uniform_structured_regular(
        volume,
        vec3f{(float)(index.x << CELL_WIDTH_BITCOUNT),
              (float)(index.y << CELL_WIDTH_BITCOUNT),
              (float)(index.z << CELL_WIDTH_BITCOUNT)},
        lower);

    /* coordinates of the upper corner of the cell in object coordinates */
    vec3f upper;
    transformLocalToObject_uniform_structured_regular(
        volume,
        vec3f{(float)((index.x + 1) << CELL_WIDTH_BITCOUNT),
              (float)((index.y + 1) << CELL_WIDTH_BITCOUNT),
              (float)((index.z + 1) << CELL_WIDTH_BITCOUNT)},
        upper);

    return box3f(lower, upper);
  }

  inline bool GridAccelerator_nextCell(const GridAccelerator *accelerator,
                                       GridAcceleratorIterator *iterator,
                                       vec3i &cellIndex,
                                       box1f &cellTRange)
  {
    SharedStructuredVolume *volume = accelerator->volume;

    const bool firstCell = cellIndex.x == -1;
    box1f cellInterval;

    if (firstCell) {
      /* first iteration */
      vec3f localCoordinates;

      transformObjectToLocal_uniform_structured_regular(
          volume,
          iterator->origin +
              iterator->boundingBoxTRange.lower * iterator->direction,
          localCoordinates);

      cellIndex        = vec3i{(int)localCoordinates.x >> CELL_WIDTH_BITCOUNT,
                        (int)localCoordinates.y >> CELL_WIDTH_BITCOUNT,
                        (int)localCoordinates.z >> CELL_WIDTH_BITCOUNT};
      box3f cellBounds = GridAccelerator_getCellBounds(accelerator, cellIndex);

      /* clamp next cell bounds to ray iterator bounding range */
      cellInterval = intersectBox(iterator->origin,
                                  iterator->direction,
                                  cellBounds,
                                  iterator->boundingBoxTRange);
    }

    if (!firstCell || isempty1f(cellInterval)) {
      /* subsequent iterations: only moving one cell at a time */

      /* TODO: see "A Fast Voxel Traversal Algorithm for Ray Tracing", John
         Amanatides, to see if this can be further simplified */

      /* transform object-space direction and origin to cell-space */
      const vec3f cellDirection = vec3f{
          iterator->direction.x * 1.f / volume->gridSpacing.x * RCP_CELL_WIDTH,
          iterator->direction.y * 1.f / volume->gridSpacing.y * RCP_CELL_WIDTH,
          iterator->direction.z * 1.f / volume->gridSpacing.z * RCP_CELL_WIDTH};

      const vec3f rcpCellDirection = divide_safe(cellDirection);

      vec3f cellOrigin;

      transformObjectToLocal_uniform_structured_regular(
          volume, iterator->origin, cellOrigin);
      cellOrigin = cellOrigin * RCP_CELL_WIDTH;

      /* sign of direction determines index delta (1 or -1 in each
         dimension) to far corner cell */
      const vec3i cornerDeltaCellIndex =
          vec3i{static_cast<int>(1 - 2 * (intbits(cellDirection.x) >> 31)),
                static_cast<int>(1 - 2 * (intbits(cellDirection.y) >> 31)),
                static_cast<int>(1 - 2 * (intbits(cellDirection.z) >> 31))};

      /* find exit distance within current cell */
      const vec3f t0 = (vec3f{static_cast<float>(cellIndex.x),
                              static_cast<float>(cellIndex.y),
                              static_cast<float>(cellIndex.z)} -
                        cellOrigin) *
                       rcpCellDirection;
      const vec3f t1 = (vec3f{static_cast<float>(cellIndex.x + 1),
                              static_cast<float>(cellIndex.y + 1),
                              static_cast<float>(cellIndex.z + 1)} -
                        cellOrigin) *
                       rcpCellDirection;
      const vec3f tMax = max(t0, t1);

      const float tExit = reduce_min(tMax);

      /* the next cell corresponds to the exit point (which will be a
         movement in one direction only) */
      vec3i deltaCellIndex =
          vec3i{tMax.x == tExit ? cornerDeltaCellIndex.x : 0,
                tMax.y == tExit ? cornerDeltaCellIndex.y : 0,
                tMax.z == tExit ? cornerDeltaCellIndex.z : 0};

      cellIndex = cellIndex + deltaCellIndex;

      box3f cellBounds = GridAccelerator_getCellBounds(accelerator, cellIndex);

      /* clamp next cell bounds to ray iterator bounding range */
      cellInterval = intersectBox(iterator->origin,
                                  iterator->direction,
                                  cellBounds,
                                  iterator->boundingBoxTRange);
    }

    if (isempty1f(cellInterval)) {
      cellTRange = box1f();
      return false;
    } else {
      cellTRange = cellInterval;
      return true;
    }
  }

  // from GridAcceleratorIterator.ispc

  inline void GridAcceleratorIterator_iterateInterval_uniform(
      const void *_self, const void *_interval, int *result)
  {
    GridAcceleratorIterator *self = (GridAcceleratorIterator *)_self;
    Interval *interval            = (Interval *)_interval;

    if (isempty1f(self->boundingBoxTRange)) {
      *result = false;
      return;
    }

    const SharedStructuredVolume *volume =
        (const SharedStructuredVolume *)
            self->super.context->super.sampler->volume;

    while (GridAccelerator_nextCell(volume->accelerator,
                                    self,
                                    self->intervalState.currentCellIndex,
                                    interval->tRange)) {
      box1f cellValueRange;

      GridAccelerator_getCellValueRange(
          volume->accelerator,
          self->intervalState.currentCellIndex,
          self->super.context->super.attributeIndex,
          cellValueRange);
      bool returnInterval = false;

      if (valueRangesOverlap(self->super.context->super.valueRanges,
                             cellValueRange)) {
        returnInterval = true;
      }

      if (returnInterval) {
        interval->valueRange    = cellValueRange;
        interval->nominalDeltaT = self->intervalState.nominalDeltaT;

        *result = true;
        return;
      }
    }

    *result = false;
  }

}  // namespace ispc
