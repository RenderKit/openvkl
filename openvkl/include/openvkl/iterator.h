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
#include "value_selector.h"
#include "volume.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// Interval iterators /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VKLIntervalIterator
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE];
  VKLVolume volume;
};

struct VKLIntervalIterator4
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_4) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE_4];
  VKLVolume volume;
};

struct VKLIntervalIterator8
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_8) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE_8];
  VKLVolume volume;
};

struct VKLIntervalIterator16
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_16) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE_16];
  VKLVolume volume;
};

struct VKLInterval
{
  vkl_range1f tRange;
  vkl_range1f valueRange;
  float nominalDeltaT;
};

struct VKLInterval4
{
  vkl_vrange1f4 tRange;
  vkl_vrange1f4 valueRange;
  float nominalDeltaT[4];
};

struct VKLInterval8
{
  vkl_vrange1f8 tRange;
  vkl_vrange1f8 valueRange;
  float nominalDeltaT[8];
};

struct VKLInterval16
{
  vkl_vrange1f16 tRange;
  vkl_vrange1f16 valueRange;
  float nominalDeltaT[16];
};

void vklInitIntervalIterator(struct VKLIntervalIterator *iterator,
                             VKLVolume volume,
                             const vkl_vec3f *origin,
                             const vkl_vec3f *direction,
                             const vkl_range1f *tRange,
                             VKLValueSelector valueSelector);

void vklInitIntervalIterator4(const int *valid,
                              struct VKLIntervalIterator4 *iterator,
                              VKLVolume volume,
                              const vkl_vvec3f4 *origin,
                              const vkl_vvec3f4 *direction,
                              const vkl_vrange1f4 *tRange,
                              VKLValueSelector valueSelector);

void vklInitIntervalIterator8(const int *valid,
                              struct VKLIntervalIterator8 *iterator,
                              VKLVolume volume,
                              const vkl_vvec3f8 *origin,
                              const vkl_vvec3f8 *direction,
                              const vkl_vrange1f8 *tRange,
                              VKLValueSelector valueSelector);

void vklInitIntervalIterator16(const int *valid,
                               struct VKLIntervalIterator16 *iterator,
                               VKLVolume volume,
                               const vkl_vvec3f16 *origin,
                               const vkl_vvec3f16 *direction,
                               const vkl_vrange1f16 *tRange,
                               VKLValueSelector valueSelector);

// returns true while the iterator is still within the volume
int vklIterateInterval(struct VKLIntervalIterator *iterator,
                       struct VKLInterval *interval);

void vklIterateInterval4(const int *valid,
                         struct VKLIntervalIterator4 *iterator,
                         struct VKLInterval4 *interval,
                         int *result);

void vklIterateInterval8(const int *valid,
                         struct VKLIntervalIterator8 *iterator,
                         struct VKLInterval8 *interval,
                         int *result);

void vklIterateInterval16(const int *valid,
                          struct VKLIntervalIterator16 *iterator,
                          struct VKLInterval16 *interval,
                          int *result);

///////////////////////////////////////////////////////////////////////////////
// Hit iterators //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VKLHitIterator
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE];
  VKLVolume volume;
};

struct VKLHitIterator4
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_4) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE_4];
  VKLVolume volume;
};

struct VKLHitIterator8
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_8) char internalState
      [ITERATOR_INTERNAL_STATE_SIZE_8];
  VKLVolume volume;
};

struct VKLHitIterator16
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_16) char internalState
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

void vklInitHitIterator(struct VKLHitIterator *iterator,
                        VKLVolume volume,
                        const vkl_vec3f *origin,
                        const vkl_vec3f *direction,
                        const vkl_range1f *tRange,
                        VKLValueSelector valueSelector);

void vklInitHitIterator4(const int *valid,
                         struct VKLHitIterator4 *iterator,
                         VKLVolume volume,
                         const vkl_vvec3f4 *origin,
                         const vkl_vvec3f4 *direction,
                         const vkl_vrange1f4 *tRange,
                         VKLValueSelector valueSelector);

void vklInitHitIterator8(const int *valid,
                         struct VKLHitIterator8 *iterator,
                         VKLVolume volume,
                         const vkl_vvec3f8 *origin,
                         const vkl_vvec3f8 *direction,
                         const vkl_vrange1f8 *tRange,
                         VKLValueSelector valueSelector);

void vklInitHitIterator16(const int *valid,
                          struct VKLHitIterator16 *iterator,
                          VKLVolume volume,
                          const vkl_vvec3f16 *origin,
                          const vkl_vvec3f16 *direction,
                          const vkl_vrange1f16 *tRange,
                          VKLValueSelector valueSelector);

// returns true while the iterator is still within the volume
int vklIterateHit(struct VKLHitIterator *iterator, struct VKLHit *hit);

void vklIterateHit4(const int *valid,
                    struct VKLHitIterator4 *iterator,
                    struct VKLHit4 *hit,
                    int *result);

void vklIterateHit8(const int *valid,
                    struct VKLHitIterator8 *iterator,
                    struct VKLHit8 *hit,
                    int *result);

void vklIterateHit16(const int *valid,
                     struct VKLHitIterator16 *iterator,
                     struct VKLHit16 *hit,
                     int *result);

#ifdef __cplusplus
}  // extern "C"
#endif
