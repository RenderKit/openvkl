// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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

typedef struct
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT)
  char internalState[ITERATOR_INTERNAL_STATE_SIZE];
  VKLVolume volume;
} VKLIntervalIterator;

typedef struct
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_4)
  char internalState[ITERATOR_INTERNAL_STATE_SIZE_4];
  VKLVolume volume;
} VKLIntervalIterator4;

typedef struct
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_8)
  char internalState[ITERATOR_INTERNAL_STATE_SIZE_8];
  VKLVolume volume;
} VKLIntervalIterator8;

typedef struct
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_16)
  char internalState[ITERATOR_INTERNAL_STATE_SIZE_16];
  VKLVolume volume;
} VKLIntervalIterator16;

typedef struct
{
  vkl_range1f tRange;
  vkl_range1f valueRange;
  float nominalDeltaT;
} VKLInterval;

typedef struct
{
  vkl_vrange1f4 tRange;
  vkl_vrange1f4 valueRange;
  float nominalDeltaT[4];
} VKLInterval4;

typedef struct
{
  vkl_vrange1f8 tRange;
  vkl_vrange1f8 valueRange;
  float nominalDeltaT[8];
} VKLInterval8;

typedef struct
{
  vkl_vrange1f16 tRange;
  vkl_vrange1f16 valueRange;
  float nominalDeltaT[16];
} VKLInterval16;

OPENVKL_INTERFACE
void vklInitIntervalIterator(VKLIntervalIterator *iterator,
                             VKLVolume volume,
                             const vkl_vec3f *origin,
                             const vkl_vec3f *direction,
                             const vkl_range1f *tRange,
                             VKLValueSelector valueSelector);

OPENVKL_INTERFACE
void vklInitIntervalIterator4(const int *valid,
                              VKLIntervalIterator4 *iterator,
                              VKLVolume volume,
                              const vkl_vvec3f4 *origin,
                              const vkl_vvec3f4 *direction,
                              const vkl_vrange1f4 *tRange,
                              VKLValueSelector valueSelector);

OPENVKL_INTERFACE
void vklInitIntervalIterator8(const int *valid,
                              VKLIntervalIterator8 *iterator,
                              VKLVolume volume,
                              const vkl_vvec3f8 *origin,
                              const vkl_vvec3f8 *direction,
                              const vkl_vrange1f8 *tRange,
                              VKLValueSelector valueSelector);

OPENVKL_INTERFACE
void vklInitIntervalIterator16(const int *valid,
                               VKLIntervalIterator16 *iterator,
                               VKLVolume volume,
                               const vkl_vvec3f16 *origin,
                               const vkl_vvec3f16 *direction,
                               const vkl_vrange1f16 *tRange,
                               VKLValueSelector valueSelector);

// returns true while the iterator is still within the volume
OPENVKL_INTERFACE
int vklIterateInterval(VKLIntervalIterator *iterator, VKLInterval *interval);

OPENVKL_INTERFACE
void vklIterateInterval4(const int *valid,
                         VKLIntervalIterator4 *iterator,
                         VKLInterval4 *interval,
                         int *result);

OPENVKL_INTERFACE
void vklIterateInterval8(const int *valid,
                         VKLIntervalIterator8 *iterator,
                         VKLInterval8 *interval,
                         int *result);

OPENVKL_INTERFACE
void vklIterateInterval16(const int *valid,
                          VKLIntervalIterator16 *iterator,
                          VKLInterval16 *interval,
                          int *result);

///////////////////////////////////////////////////////////////////////////////
// Hit iterators //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT)
  char internalState[ITERATOR_INTERNAL_STATE_SIZE];
  VKLVolume volume;
} VKLHitIterator;

typedef struct
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_4)
  char internalState[ITERATOR_INTERNAL_STATE_SIZE_4];
  VKLVolume volume;
} VKLHitIterator4;

typedef struct
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_8)
  char internalState[ITERATOR_INTERNAL_STATE_SIZE_8];
  VKLVolume volume;
} VKLHitIterator8;

typedef struct
{
  VKL_ALIGN(ITERATOR_INTERNAL_STATE_ALIGNMENT_16)
  char internalState[ITERATOR_INTERNAL_STATE_SIZE_16];
  VKLVolume volume;
} VKLHitIterator16;

typedef struct
{
  float t;
  float sample;
} VKLHit;

typedef struct
{
  float t[4];
  float sample[4];
} VKLHit4;

typedef struct
{
  float t[8];
  float sample[8];
} VKLHit8;

typedef struct
{
  float t[16];
  float sample[16];
} VKLHit16;

OPENVKL_INTERFACE
void vklInitHitIterator(VKLHitIterator *iterator,
                        VKLVolume volume,
                        const vkl_vec3f *origin,
                        const vkl_vec3f *direction,
                        const vkl_range1f *tRange,
                        VKLValueSelector valueSelector);

OPENVKL_INTERFACE
void vklInitHitIterator4(const int *valid,
                         VKLHitIterator4 *iterator,
                         VKLVolume volume,
                         const vkl_vvec3f4 *origin,
                         const vkl_vvec3f4 *direction,
                         const vkl_vrange1f4 *tRange,
                         VKLValueSelector valueSelector);

OPENVKL_INTERFACE
void vklInitHitIterator8(const int *valid,
                         VKLHitIterator8 *iterator,
                         VKLVolume volume,
                         const vkl_vvec3f8 *origin,
                         const vkl_vvec3f8 *direction,
                         const vkl_vrange1f8 *tRange,
                         VKLValueSelector valueSelector);

OPENVKL_INTERFACE
void vklInitHitIterator16(const int *valid,
                          VKLHitIterator16 *iterator,
                          VKLVolume volume,
                          const vkl_vvec3f16 *origin,
                          const vkl_vvec3f16 *direction,
                          const vkl_vrange1f16 *tRange,
                          VKLValueSelector valueSelector);

// returns true while the iterator is still within the volume
OPENVKL_INTERFACE
int vklIterateHit(VKLHitIterator *iterator, VKLHit *hit);

OPENVKL_INTERFACE
void vklIterateHit4(const int *valid,
                    VKLHitIterator4 *iterator,
                    VKLHit4 *hit,
                    int *result);

OPENVKL_INTERFACE
void vklIterateHit8(const int *valid,
                    VKLHitIterator8 *iterator,
                    VKLHit8 *hit,
                    int *result);

OPENVKL_INTERFACE
void vklIterateHit16(const int *valid,
                     VKLHitIterator16 *iterator,
                     VKLHit16 *hit,
                     int *result);

#ifdef __cplusplus
}  // extern "C"
#endif
