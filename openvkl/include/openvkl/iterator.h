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
#include "iterator_size.h"
#include "samples_mask.h"
#include "volume.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// Interval iterators /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VKLIntervalIterator
{
  alignas(ITERATOR_INTERNAL_STATE_ALIGNMENT) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE];
  VKLVolume volume;
};

struct VKLIntervalIterator4
{
  alignas(ITERATOR_INTERNAL_STATE_ALIGNMENT_4) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE_4];
  VKLVolume volume;
};

struct VKLIntervalIterator8
{
  alignas(ITERATOR_INTERNAL_STATE_ALIGNMENT_8) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE_8];
  VKLVolume volume;
};

struct VKLIntervalIterator16
{
  alignas(ITERATOR_INTERNAL_STATE_ALIGNMENT_16) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE_16];
  VKLVolume volume;
};

struct VKLInterval
{
  vkl_range1f tRange;
  float nominalDeltaT;
};

struct VKLInterval4
{
  vkl_vrange1f4 tRange;
  float nominalDeltaT[4];
};

struct VKLInterval8
{
  vkl_vrange1f8 tRange;
  float nominalDeltaT[8];
};

struct VKLInterval16
{
  vkl_vrange1f16 tRange;
  float nominalDeltaT[16];
};

void vklInitIntervalIterator(VKLIntervalIterator *iterator,
                             VKLVolume volume,
                             const vkl_vec3f *origin,
                             const vkl_vec3f *direction,
                             const vkl_range1f *tRange,
                             VKLSamplesMask samplesMask);

void vklInitIntervalIterator4(const int *valid,
                              VKLIntervalIterator4 *iterator,
                              VKLVolume volume,
                              const vkl_vvec3f4 *origin,
                              const vkl_vvec3f4 *direction,
                              const vkl_vrange1f4 *tRange,
                              VKLSamplesMask samplesMask);

void vklInitIntervalIterator8(const int *valid,
                              VKLIntervalIterator8 *iterator,
                              VKLVolume volume,
                              const vkl_vvec3f8 *origin,
                              const vkl_vvec3f8 *direction,
                              const vkl_vrange1f8 *tRange,
                              VKLSamplesMask samplesMask);

void vklInitIntervalIterator16(const int *valid,
                               VKLIntervalIterator16 *iterator,
                               VKLVolume volume,
                               const vkl_vvec3f16 *origin,
                               const vkl_vvec3f16 *direction,
                               const vkl_vrange1f16 *tRange,
                               VKLSamplesMask samplesMask);

// returns true while the iterator is still within the volume
bool vklIterateInterval(VKLIntervalIterator *iterator, VKLInterval *interval);

void vklIterateInterval4(const int *valid,
                         VKLIntervalIterator4 *iterator,
                         VKLInterval4 *interval,
                         int *result);

void vklIterateInterval8(const int *valid,
                         VKLIntervalIterator8 *iterator,
                         VKLInterval8 *interval,
                         int *result);

void vklIterateInterval16(const int *valid,
                          VKLIntervalIterator16 *iterator,
                          VKLInterval16 *interval,
                          int *result);

///////////////////////////////////////////////////////////////////////////////
// Hit iterators //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VKLHitIterator
{
  alignas(ITERATOR_INTERNAL_STATE_ALIGNMENT) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE];
  VKLVolume volume;
};

struct VKLHitIterator4
{
  alignas(ITERATOR_INTERNAL_STATE_ALIGNMENT_4) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE_4];
  VKLVolume volume;
};

struct VKLHitIterator8
{
  alignas(ITERATOR_INTERNAL_STATE_ALIGNMENT_8) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE_8];
  VKLVolume volume;
};

struct VKLHitIterator16
{
  alignas(ITERATOR_INTERNAL_STATE_ALIGNMENT_16) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE_16];
  VKLVolume volume;
};

struct VKLHit
{
  float t;
  float sample;
};

struct VKLHit4
{
  float t[4];
  float sample[4];
};

struct VKLHit8
{
  float t[8];
  float sample[8];
};

struct VKLHit16
{
  float t[16];
  float sample[16];
};

void vklInitHitIterator(VKLHitIterator *iterator,
                        VKLVolume volume,
                        const vkl_vec3f *origin,
                        const vkl_vec3f *direction,
                        const vkl_range1f *tRange,
                        VKLSamplesMask samplesMask);

void vklInitHitIterator4(const int *valid,
                         VKLHitIterator4 *iterator,
                         VKLVolume volume,
                         const vkl_vvec3f4 *origin,
                         const vkl_vvec3f4 *direction,
                         const vkl_vrange1f4 *tRange,
                         VKLSamplesMask samplesMask);

void vklInitHitIterator8(const int *valid,
                         VKLHitIterator8 *iterator,
                         VKLVolume volume,
                         const vkl_vvec3f8 *origin,
                         const vkl_vvec3f8 *direction,
                         const vkl_vrange1f8 *tRange,
                         VKLSamplesMask samplesMask);

void vklInitHitIterator16(const int *valid,
                          VKLHitIterator16 *iterator,
                          VKLVolume volume,
                          const vkl_vvec3f16 *origin,
                          const vkl_vvec3f16 *direction,
                          const vkl_vrange1f16 *tRange,
                          VKLSamplesMask samplesMask);

// returns true while the iterator is still within the volume
bool vklIterateHit(VKLHitIterator *iterator, VKLHit *hit);

void vklIterateHit4(const int *valid,
                    VKLHitIterator4 *iterator,
                    VKLHit4 *hit,
                    int *result);

void vklIterateHit8(const int *valid,
                    VKLHitIterator8 *iterator,
                    VKLHit8 *hit,
                    int *result);

void vklIterateHit16(const int *valid,
                     VKLHitIterator16 *iterator,
                     VKLHit16 *hit,
                     int *result);

#ifdef __cplusplus
}  // extern "C"
#endif
