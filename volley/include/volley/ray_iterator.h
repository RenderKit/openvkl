// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#pragma once

#include "common.h"
#include "samples_mask.h"
#include "volume.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RayIterator : public ManagedObject
{
};

typedef RayIterator *VLYRayIterator;

VLYRayIterator vlyNewRayIterator(VLYVolume volume,
                                 const vly_vec3f *origin,
                                 const vly_vec3f *direction,
                                 const vly_range1f *tRange,
                                 VLYSamplesMask samplesMask);

struct VLYRayInterval
{
  vly_range1f tRange;
  float nominalDeltaT;
  // TODO: output samples mask when needed
};

struct VLYRayInterval4
{
  vly_vrange1f4 tRange;
  float nominalDeltaT[4];
  // TODO: output samples mask when needed
};

struct VLYRayInterval8
{
  vly_vrange1f8 tRange;
  float nominalDeltaT[8];
};

struct VLYRayInterval16
{
  vly_vrange1f16 tRange;
  float nominalDeltaT[16];
};

// returns true while the iterator is still within the volume
bool vlyIterateInterval(VLYRayIterator *rayIterator,
                        VLYRayInterval *rayInterval);

struct VLYSurfaceHit
{
  float t;
  float sample;
};

struct VLYSurfaceHit4
{
  float t[4];
  float sample[4];
};

struct VLYSurfaceHit8
{
  float t[8];
  float sample[8];
};

struct VLYSurfaceHit16
{
  float t[16];
  float sample[16];
};

// returns true while the iterator is still within the volume
bool vlyIterateSurface(VLYRayIterator *rayIterator, VLYSurfaceHit *surfaceHit);

/* TODO:

- need nominalDeltaT to also be returned
- open whether output sample mask is returned
*/

#ifdef __cplusplus
}  // extern "C"
#endif
