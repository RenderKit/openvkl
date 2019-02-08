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

#include "volley_common.h"
#include "volley_samples_mask.h"
#include "volley_volume.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RayIterator;
typedef RayIterator *VLYRayIterator;

VLYRayIterator vlyNewRayIterator(VLYVolume volume,
                                 const vly_vec3f *origin,
                                 const vly_vec3f *direction,
                                 const vly_range1f *tRange,
                                 VLYSamplesMask samplesMask);

// returns true while the iterator is still within the volume
bool vlyIterateInterval(VLYRayIterator rayIterator,
                        vly_range1f *tRange,
                        VLYSamplesMask *intervalSamplesMask);

// returns true while the iterator is still within the volume
bool vlyIterateSurface(VLYRayIterator, float *t, float *sample);

/* TODO:

- need nominalDeltaT to also be returned
- open whether output sample mask is returned
*/

#ifdef __cplusplus
}  // extern "C"
#endif
