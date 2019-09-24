// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "../math/box_utility.ih"
#include "../value_selector/ValueSelector.ih"
#include "../volume/GridAccelerator.ih"
#include "../volume/SharedStructuredVolume.ih"
#include "GridAcceleratorIterator.ih"

inline void printGridAcceleratorIterator(
    varying GridAcceleratorIterator *uniform self)
{
  print("origin:\n  %\n  %\n  %\n",
        self->origin.x,
        self->origin.y,
        self->origin.z);
  print("direction:\n  %\n  %\n  %\n",
        self->direction.x,
        self->direction.y,
        self->direction.z);
  print("tRange\n  %\n  %\n", self->tRange.lower, self->tRange.upper);
  print("boundingBoxTRange:\n  %\n  %\n",
        self->boundingBoxTRange.lower,
        self->boundingBoxTRange.upper);
}

export uniform int GridAcceleratorIterator_sizeOf()
{
  return sizeof(varying GridAcceleratorIterator);
}

// for tests only
export void *uniform GridAcceleratorIterator_new()
{
  return uniform new varying GridAcceleratorIterator;
}

export void GridAcceleratorIterator_Initialize(const int *uniform imask,
                                               void *uniform _self,
                                               void *uniform _volume,
                                               void *uniform _origin,
                                               void *uniform _direction,
                                               void *uniform _tRange,
                                               void *uniform _valueSelector)
{
  if (!imask[programIndex]) {
    return;
  }

  varying GridAcceleratorIterator *uniform self =
      (varying GridAcceleratorIterator * uniform) _self;

  self->volume        = (uniform SharedStructuredVolume * uniform) _volume;
  self->origin        = *((varying vec3f * uniform) _origin);
  self->direction     = *((varying vec3f * uniform) _direction);
  self->tRange        = *((varying box1f * uniform) _tRange);
  self->valueSelector = (uniform ValueSelector * uniform) _valueSelector;

  self->boundingBoxTRange = intersectBox(
      self->origin, self->direction, self->volume->boundingBox, self->tRange);

  // if using ISPC fast-math and approximate rcp() functions, an epsilon needs
  // to be added to the bounding box intersection to prevent artifacts. this is
  // because the inverse coordinate transformation may land outside a grid cell.
  // the artifacts may look like speckles, or cross-hairs in orthographic camera
  // views.
  /*const uniform float epsilon = reduce_min(self->volume->gridSpacing) *
  0.125f;

  self->boundingBoxTRange.lower += epsilon;*/

  resetInterval(self->intervalState.currentInterval);
  self->intervalState.currentCellIndex = make_vec3i(-1);

  self->hitState.currentCellIndex  = make_vec3i(-1);
  self->hitState.currentCellTRange = make_box1f(inf, -inf);
}

export void *uniform
GridAcceleratorIterator_getCurrentInterval(void *uniform _self)
{
  varying GridAcceleratorIterator *uniform self =
      (varying GridAcceleratorIterator * uniform) _self;
  return &self->intervalState.currentInterval;
}

export void GridAcceleratorIterator_iterateInterval(
    const int *uniform imask, void *uniform _self, uniform int *uniform _result)
{
  if (!imask[programIndex]) {
    return;
  }

  varying GridAcceleratorIterator *uniform self =
      (varying GridAcceleratorIterator * uniform) _self;

  varying int *uniform result = (varying int *uniform)_result;

  if (isempty1f(self->boundingBoxTRange)) {
    *result = false;
    return;
  }

  while (GridAccelerator_nextCell(self->volume->accelerator,
                                  self,
                                  self->intervalState.currentCellIndex,
                                  self->intervalState.currentInterval.tRange)) {
    box1f cellValueRange;
    GridAccelerator_getCellValueRange(self->volume->accelerator,
                                      self->intervalState.currentCellIndex,
                                      cellValueRange);

    bool returnInterval = false;

    if (!self->valueSelector) {
      returnInterval = true;
    } else {
      if (overlaps1f(self->valueSelector->rangesMinMax, cellValueRange)) {
        if (overlapsAny1f(cellValueRange,
                          self->valueSelector->numRanges,
                          self->valueSelector->ranges)) {
          returnInterval = true;
        }
      }
    }

    if (returnInterval) {
      self->intervalState.currentInterval.valueRange = cellValueRange;
      self->intervalState.currentInterval.nominalDeltaT =
          1.f * (self->intervalState.currentInterval.tRange.upper -
                 self->intervalState.currentInterval.tRange.lower);

      *result = true;
      return;
    }
  }

  *result = false;
}

export void *uniform GridAcceleratorIterator_getCurrentHit(void *uniform _self)
{
  varying GridAcceleratorIterator *uniform self =
      (varying GridAcceleratorIterator * uniform) _self;
  return &self->hitState.currentHit;
}

export void GridAcceleratorIterator_iterateHit(const int *uniform imask,
                                               void *uniform _self,
                                               uniform int *uniform _result)
{
  if (!imask[programIndex]) {
    return;
  }

  varying GridAcceleratorIterator *uniform self =
      (varying GridAcceleratorIterator * uniform) _self;

  varying int *uniform result = (varying int *uniform)_result;

  if (isempty1f(self->boundingBoxTRange)) {
    *result = false;
    return;
  }

  cif(!self->valueSelector || self->valueSelector->numValues == 0)
  {
    *result = false;
    return;
  }

  // first iteration
  cif(self->hitState.currentCellIndex.x == -1)
  {
    self->hitState.activeCell =
        GridAccelerator_nextCell(self->volume->accelerator,
                                 self,
                                 self->hitState.currentCellIndex,
                                 self->hitState.currentCellTRange);
  }

  const uniform float step = reduce_min(self->volume->gridSpacing);

  while (self->hitState.activeCell) {
    box1f cellValueRange;
    GridAccelerator_getCellValueRange(self->volume->accelerator,
                                      self->hitState.currentCellIndex,
                                      cellValueRange);

    bool cellValueRangeOverlap =
        overlaps1f(self->valueSelector->valuesMinMax, cellValueRange);

    if (cellValueRangeOverlap) {
      float surfaceEpsilon;

      bool foundHit = intersectSurfaces(&self->volume->super,
                                        self->origin,
                                        self->direction,
                                        self->hitState.currentCellTRange,
                                        step,
                                        self->valueSelector->numValues,
                                        self->valueSelector->values,
                                        self->hitState.currentHit,
                                        surfaceEpsilon);

      if (foundHit) {
        *result = true;
        self->hitState.currentCellTRange.lower =
            self->hitState.currentHit.t + surfaceEpsilon;

        // move to next cell if next t passes the cell boundary
        if (isempty1f(self->hitState.currentCellTRange)) {
          self->hitState.activeCell =
              GridAccelerator_nextCell(self->volume->accelerator,
                                       self,
                                       self->hitState.currentCellIndex,
                                       self->hitState.currentCellTRange);

          // continue where we left off
          self->hitState.currentCellTRange.lower =
              self->hitState.currentHit.t + surfaceEpsilon;
        }

        return;
      }
    }

    // if no hits are found, move to the next cell; if a hit is found we'll stay
    // in the cell to pursue other hits
    self->hitState.activeCell =
        GridAccelerator_nextCell(self->volume->accelerator,
                                 self,
                                 self->hitState.currentCellIndex,
                                 self->hitState.currentCellTRange);
  }

  *result = false;
}
