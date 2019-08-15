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
#include "../samples_mask/GridAcceleratorSamplesMask.ih"
#include "../volume/GridAccelerator.ih"
#include "GridAcceleratorRayIterator.ih"

inline void printGridAcceleratorRayIterator(
    uniform GridAcceleratorRayIterator *uniform self)
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

export uniform int GridAcceleratorRayIterator_sizeOf()
{
  return sizeof(uniform GridAcceleratorRayIterator);
}

// for tests only
export uniform GridAcceleratorRayIterator *uniform
GridAcceleratorRayIterator_new()
{
  return uniform new uniform GridAcceleratorRayIterator;
}

export void GridAcceleratorRayIterator_Initialize(void *uniform _self,
                                                  void *uniform _volume,
                                                  void *uniform _origin,
                                                  void *uniform _direction,
                                                  void *uniform _tRange,
                                                  void *uniform _samplesMask)
{
  uniform GridAcceleratorRayIterator *uniform self =
      (uniform GridAcceleratorRayIterator * uniform) _self;

  self->volume    = (uniform SharedStructuredVolume * uniform) _volume;
  self->origin    = *((varying vec3f * uniform) _origin);
  self->direction = *((varying vec3f * uniform) _direction);
  self->tRange    = *((varying box1f * uniform) _tRange);
  self->samplesMask =
      (uniform GridAcceleratorSamplesMask * uniform) _samplesMask;

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

  resetInterval(self->hitState.currentInterval);
  self->hitState.currentCellIndex = make_vec3i(-1);
}

export void *uniform
GridAcceleratorRayIterator_getCurrentInterval(void *uniform _self)
{
  uniform GridAcceleratorRayIterator *uniform self =
      (uniform GridAcceleratorRayIterator * uniform) _self;
  return &self->intervalState.currentInterval;
}

export void GridAcceleratorRayIterator_iterateInterval(
    uniform const int *uniform imask,
    void *uniform _self,
    uniform int *uniform _result)
{
  uniform GridAcceleratorRayIterator *uniform self =
      (uniform GridAcceleratorRayIterator * uniform) _self;

  varying int *uniform result = (varying int *uniform)_result;

  if (!imask[programIndex]) {
    return;
  }

  if (isempty1f(self->boundingBoxTRange)) {
    *result = false;
    return;
  }

  while (GridAccelerator_nextCell(self->volume->accelerator,
                                  self,
                                  self->intervalState.currentCellIndex,
                                  self->intervalState.currentInterval)) {
    cif(!self->samplesMask)
    {
      self->intervalState.currentInterval.nominalDeltaT =
          1.f * (self->intervalState.currentInterval.tRange.upper -
                 self->intervalState.currentInterval.tRange.lower);

      *result = true;
      return;
    }

    box1f cellValueRange;
    GridAccelerator_getCellValueRange(self->volume->accelerator,
                                      self->intervalState.currentCellIndex,
                                      cellValueRange);

    if (overlaps1f(self->samplesMask->rangesMinMax, cellValueRange)) {
      self->intervalState.currentInterval.nominalDeltaT =
          1.f * (self->intervalState.currentInterval.tRange.upper -
                 self->intervalState.currentInterval.tRange.lower);

      *result = true;
      return;
    }
  }

  *result = false;
}

export void *uniform
GridAcceleratorRayIterator_getCurrentHit(void *uniform _self)
{
  uniform GridAcceleratorRayIterator *uniform self =
      (uniform GridAcceleratorRayIterator * uniform) _self;
  return &self->hitState.currentHit;
}

inline bool intersectSurfaces(const SharedStructuredVolume *uniform volume,
                              const varying vec3f &origin,
                              const varying vec3f &direction,
                              const varying Interval &interval,
                              const uniform int numValues,
                              const float *uniform values,
                              varying Hit &hit,
                              varying float &surfaceEpsilon)
{
  const uniform float step = reduce_min(volume->gridSpacing);

  float t0      = interval.tRange.lower;
  float sample0 = volume->computeSample(volume, origin + t0 * direction);

  float t;

  while (true) {
    t = t0 + step;

    if (t > interval.tRange.upper + step)
      return false;

    const float sample = volume->computeSample(volume, origin + t * direction);

    float tHit    = inf;
    float epsilon = inf;
    float value   = inf;

    if (!isnan(sample0 + sample) && (sample != sample0)) {
      for (uniform int i = 0; i < numValues; i++) {
        if ((values[i] - sample0) * (values[i] - sample) <= 0.f) {
          const float rcpSamp = 1.f / (sample - sample0);
          float tIso          = inf;
          if (!isnan(rcpSamp)) {
            tIso = t0 + (values[i] - sample0) * rcpSamp * (t - t0);
          }

          if (tIso < tHit && tIso >= interval.tRange.lower) {
            tHit    = tIso;
            value   = values[i];
            epsilon = step * 0.125f;
          }
        }
      }

      if (tHit <= interval.tRange.upper) {
        hit.t          = tHit;
        hit.sample     = value;
        surfaceEpsilon = epsilon;
        return true;
      }
    }

    t0      = t;
    sample0 = sample;
  }

  return false;
}

export void GridAcceleratorRayIterator_iterateHit(uniform const int *uniform
                                                      imask,
                                                  void *uniform _self,
                                                  uniform int *uniform _result)
{
  uniform GridAcceleratorRayIterator *uniform self =
      (uniform GridAcceleratorRayIterator * uniform) _self;

  varying int *uniform result = (varying int *uniform)_result;

  if (!imask[programIndex]) {
    return;
  }

  if (isempty1f(self->boundingBoxTRange)) {
    *result = false;
    return;
  }

  cif(!self->samplesMask || self->samplesMask->numValues == 0)
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
                                 self->hitState.currentInterval);
  }

  while (self->hitState.activeCell) {
    box1f cellValueRange;
    GridAccelerator_getCellValueRange(self->volume->accelerator,
                                      self->hitState.currentCellIndex,
                                      cellValueRange);

    bool cellValueRangeOverlap =
        overlaps1f(self->samplesMask->valuesMinMax, cellValueRange);

    if (cellValueRangeOverlap) {
      float surfaceEpsilon;

      bool foundHit = intersectSurfaces(self->volume,
                                        self->origin,
                                        self->direction,
                                        self->hitState.currentInterval,
                                        self->samplesMask->numValues,
                                        self->samplesMask->values,
                                        self->hitState.currentHit,
                                        surfaceEpsilon);

      if (foundHit) {
        *result = true;
        self->hitState.currentInterval.tRange.lower =
            self->hitState.currentHit.t + surfaceEpsilon;
        return;
      }
    }

    // we'll only move forward to the next cell if no hits are found; if a hit
    // is found we'll stay in the cell to pursue other hits
    self->hitState.activeCell =
        GridAccelerator_nextCell(self->volume->accelerator,
                                 self,
                                 self->hitState.currentCellIndex,
                                 self->hitState.currentInterval);
  }

  *result = false;
}
