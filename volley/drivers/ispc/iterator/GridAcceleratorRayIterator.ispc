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

export void *uniform
GridAcceleratorRayIterator_Constructor(void *uniform _volume,
                                       const uniform box3f &boundingBox,
                                       void *uniform _origin,
                                       void *uniform _direction,
                                       void *uniform _tRange,
                                       void *uniform _samplesMask)
{
  uniform GridAcceleratorRayIterator *uniform self =
      uniform new uniform GridAcceleratorRayIterator;

  self->volume    = (uniform SharedStructuredVolume * uniform) _volume;
  self->origin    = *((varying vec3f * uniform) _origin);
  self->direction = *((varying vec3f * uniform) _direction);
  self->tRange    = *((varying box1f * uniform) _tRange);
  self->samplesMask =
      (uniform GridAcceleratorSamplesMask * uniform) _samplesMask;

  resetRayInterval(self->currentRayInterval);

  self->boundingBoxTRange =
      intersectBox(self->origin, self->direction, boundingBox, self->tRange);

  self->currentCellIndex = make_vec3i(-1);

  return self;
}

export void *uniform GridAcceleratorRayIterator_Destructor(void *uniform _self)
{
  uniform GridAcceleratorRayIterator *uniform self =
      (uniform GridAcceleratorRayIterator * uniform) _self;

  delete self;
}

export void *uniform
GridAcceleratorRayIterator_getCurrentRayInterval(void *uniform _self)
{
  uniform GridAcceleratorRayIterator *uniform self =
      (uniform GridAcceleratorRayIterator * uniform) _self;
  return &self->currentRayInterval;
}

export void GridAcceleratorRayIterator_iterateInterval(
    uniform const int *uniform imask,
    void *uniform _self,
    uniform int *uniform _result)
{
  uniform GridAcceleratorRayIterator *uniform self =
      (uniform GridAcceleratorRayIterator * uniform) _self;

  varying int *uniform result = (varying int *uniform)_result;

  cif(imask[programIndex])
  {
    if (isempty1f(self->boundingBoxTRange)) {
      *result = false;
      return;
    }

    while (GridAccelerator_nextCell(self->volume->accelerator, self)) {
      cif(!self->samplesMask)
      {
        self->currentRayInterval.nominalDeltaT =
            1.f * (self->currentRayInterval.tRange.upper -
                   self->currentRayInterval.tRange.lower);

        *result = true;
        return;
      }

      box1f cellRange;
      GridAccelerator_getCellRange(
          self->volume->accelerator, self->currentCellIndex, cellRange);

      for (uniform int i = 0; i < self->samplesMask->numRanges; i++) {
        uniform box1f r = self->samplesMask->ranges[i];

        if (overlaps1f(r, cellRange)) {
          self->currentRayInterval.nominalDeltaT =
              1.f * (self->currentRayInterval.tRange.upper -
                     self->currentRayInterval.tRange.lower);

          *result = true;
          return;
        }
      }
    }

    *result = false;
  }
}
