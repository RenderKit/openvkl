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
#include "ray_iterator_size.h"
#include "samples_mask.h"
#include "volume.h"

#ifdef __cplusplus
extern "C" {
#endif

struct VKLRayIterator
{
  alignas(RAY_ITERATOR_INTERNAL_STATE_ALIGNMENT) char internalState
      [RAY_ITERATOR_INTERNAL_STATE_SIZE];
  VKLVolume volume;
};

struct VKLRayIterator4
{
  alignas(RAY_ITERATOR_INTERNAL_STATE_ALIGNMENT_4) char internalState
      [RAY_ITERATOR_INTERNAL_STATE_SIZE_4];
  VKLVolume volume;
};

struct VKLRayIterator8
{
  alignas(RAY_ITERATOR_INTERNAL_STATE_ALIGNMENT_8) char internalState
      [RAY_ITERATOR_INTERNAL_STATE_SIZE_8];
  VKLVolume volume;
};

struct VKLRayIterator16
{
  alignas(RAY_ITERATOR_INTERNAL_STATE_ALIGNMENT_16) char internalState
      [RAY_ITERATOR_INTERNAL_STATE_SIZE_16];
  VKLVolume volume;
};

void vklInitRayIterator(VKLRayIterator *rayIterator,
                        VKLVolume volume,
                        const vkl_vec3f *origin,
                        const vkl_vec3f *direction,
                        const vkl_range1f *tRange,
                        VKLSamplesMask samplesMask);

void vklInitRayIterator4(const int *valid,
                         VKLRayIterator4 *rayIterator,
                         VKLVolume volume,
                         const vkl_vvec3f4 *origin,
                         const vkl_vvec3f4 *direction,
                         const vkl_vrange1f4 *tRange,
                         VKLSamplesMask samplesMask);

void vklInitRayIterator8(const int *valid,
                         VKLRayIterator8 *rayIterator,
                         VKLVolume volume,
                         const vkl_vvec3f8 *origin,
                         const vkl_vvec3f8 *direction,
                         const vkl_vrange1f8 *tRange,
                         VKLSamplesMask samplesMask);

void vklInitRayIterator16(const int *valid,
                          VKLRayIterator16 *rayIterator,
                          VKLVolume volume,
                          const vkl_vvec3f16 *origin,
                          const vkl_vvec3f16 *direction,
                          const vkl_vrange1f16 *tRange,
                          VKLSamplesMask samplesMask);

struct VKLRayInterval
{
  vkl_range1f tRange;
  float nominalDeltaT;
  // TODO: output samples mask when needed
};

struct VKLRayInterval4
{
  vkl_vrange1f4 tRange;
  float nominalDeltaT[4];
  // TODO: output samples mask when needed
};

struct VKLRayInterval8
{
  vkl_vrange1f8 tRange;
  float nominalDeltaT[8];
};

struct VKLRayInterval16
{
  vkl_vrange1f16 tRange;
  float nominalDeltaT[16];
};

// returns true while the iterator is still within the volume
bool vklIterateInterval(VKLRayIterator *rayIterator,
                        VKLRayInterval *rayInterval);

void vklIterateInterval4(const int *valid,
                         VKLRayIterator4 *rayIterator,
                         VKLRayInterval4 *rayInterval,
                         int *result);

void vklIterateInterval8(const int *valid,
                         VKLRayIterator8 *rayIterator,
                         VKLRayInterval8 *rayInterval,
                         int *result);

void vklIterateInterval16(const int *valid,
                          VKLRayIterator16 *rayIterator,
                          VKLRayInterval16 *rayInterval,
                          int *result);

struct VKLSurfaceHit
{
  float t;
  float sample;
};

struct VKLSurfaceHit4
{
  float t[4];
  float sample[4];
};

struct VKLSurfaceHit8
{
  float t[8];
  float sample[8];
};

struct VKLSurfaceHit16
{
  float t[16];
  float sample[16];
};

// returns true while the iterator is still within the volume
bool vklIterateSurface(VKLRayIterator *rayIterator, VKLSurfaceHit *surfaceHit);

void vklIterateSurface4(const int *valid,
                        VKLRayIterator4 *rayIterator,
                        VKLSurfaceHit4 *surfaceHit,
                        int *result);

void vklIterateSurface8(const int *valid,
                        VKLRayIterator8 *rayIterator,
                        VKLSurfaceHit8 *surfaceHit,
                        int *result);

void vklIterateSurface16(const int *valid,
                         VKLRayIterator16 *rayIterator,
                         VKLSurfaceHit16 *surfaceHit,
                         int *result);

/* TODO:

- need nominalDeltaT to also be returned
- open whether output sample mask is returned
*/

#ifdef __cplusplus
}  // extern "C"
#endif
