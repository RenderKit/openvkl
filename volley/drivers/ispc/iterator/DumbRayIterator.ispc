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

#include "math/box.ih"
#include "math/vec.ih"
#include "volley/volley_iterator.isph"

inline varying bool isempty1f(varying box1f &box)
{
  return box.upper < box.lower;
}

struct RayInterval
{
  varying box1f tRange;
  varying float nominalDeltaT;
  // TODO: output samples mask when needed
};

struct DumbRayIterator
{
  varying vec3f origin;
  varying vec3f direction;
  varying box1f tRange;

  // computed
  varying box1f boundingBoxTRange;
  varying RayInterval currentRayInterval;
};

inline void printDumbRayIterator(uniform DumbRayIterator *uniform self)
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

inline varying box1f intersectBox(const varying vec3f &origin,
                                  const varying vec3f &direction,
                                  const uniform box3f &box,
                                  const varying box1f &rangeLimit)
{
  const varying vec3f mins = (box.lower - origin) * rcp_safe(direction);
  const varying vec3f maxs = (box.upper - origin) * rcp_safe(direction);

  varying box1f result;
  result.lower = reduce_max(make_vec4f(min(mins, maxs), rangeLimit.lower));
  result.upper = reduce_min(make_vec4f(max(mins, maxs), rangeLimit.upper));

  return result;
}

export void *uniform DumbRayIterator_create(const uniform box3f &boundingBox,
                                            void *uniform _origin,
                                            void *uniform _direction,
                                            void *uniform _tRange)
{
  uniform DumbRayIterator *uniform self = uniform new uniform DumbRayIterator;

  self->origin    = *((varying vec3f * uniform) _origin);
  self->direction = *((varying vec3f * uniform) _direction);
  self->tRange    = *((varying box1f * uniform) _tRange);

  self->boundingBoxTRange =
      intersectBox(self->origin, self->direction, boundingBox, self->tRange);

  // initial current ray interval empty
  self->currentRayInterval.tRange.lower = 1.f;
  self->currentRayInterval.tRange.lower = -1.f;

#if 0
  print("boundingBox:\n  % % %\n  % % %\n",
        boundingBox.lower.x,
        boundingBox.lower.y,
        boundingBox.lower.z,
        boundingBox.upper.x,
        boundingBox.upper.y,
        boundingBox.upper.z);

  printDumbRayIterator(self);
#endif

  return self;
}

export void DumbRayIterator_iterateInterval(uniform const int *uniform imask,
                                            uniform DumbRayIterator *uniform
                                                self,
                                            void *uniform _result)
{
  varying bool *uniform result = (varying bool *uniform)_result;

  cif(imask[programIndex])
  {
    if (isempty1f(self->boundingBoxTRange)) {
      *result = false;
    } else {
      uniform float nominalDeltaT = 0.25f;

      if (isempty1f(self->currentRayInterval.tRange)) {
        self->currentRayInterval.tRange.lower = self->boundingBoxTRange.lower;
      } else {
        self->currentRayInterval.tRange.lower += nominalDeltaT;
      }

      self->currentRayInterval.tRange.upper =
          min(self->currentRayInterval.tRange.lower + nominalDeltaT,
              self->boundingBoxTRange.upper);

      self->currentRayInterval.nominalDeltaT = 0.1f;

      *result = (self->currentRayInterval.tRange.lower <
                 self->boundingBoxTRange.upper);
    }
  }
}
