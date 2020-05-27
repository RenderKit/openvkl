// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "value_selector.h"
#include "volume.h"
#include "max_iterator_size.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// Interval iterators /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
 * Interval iterator types are opaque. See vklGetIntervalIteratorSize()
 * and vklInitializeIntervalIterator() below for more information on how
 * to allocate them.
 */

struct IntervalIterator;
typedef struct IntervalIterator *VKLIntervalIterator;

struct IntervalIterator4;
typedef struct IntervalIterator4 *VKLIntervalIterator4;

struct IntervalIterator8;
typedef struct IntervalIterator8 *VKLIntervalIterator8;

struct IntervalIterator16;
typedef struct IntervalIterator16 *VKLIntervalIterator16;

/*
 * Return the size, in bytes, required to store an interval iterator for the
 * given volumes.
 */
OPENVKL_INTERFACE
size_t vklGetIntervalIteratorSize(VKLVolume volume);

OPENVKL_INTERFACE
size_t vklGetIntervalIteratorSize4(VKLVolume volume);

OPENVKL_INTERFACE
size_t vklGetIntervalIteratorSize8(VKLVolume volume);

OPENVKL_INTERFACE
size_t vklGetIntervalIteratorSize16(VKLVolume volume);

/*
 * Initialize an interval iterator for the given volume.
 *
 * The buffer must be at leaset vklGetIntervalIteratorSize*() bytes in size.
 * The returned iterator must not be used after the lifetime of the
 * buffer ends.
 *
 * Users should take care to make buffer allocation as fast as possible.
 * One way to do this is stack allocation using alloca():
 *
 * void *buffer = alloca(vklGetIntervalIteratorSize(volume);
 * VKLIntervalIterator it = vklInitializeIntervalIterator(volume, buffer);
 *
 * If done this way, remember that buffer will be destroyed at the end of the
 * current scope.
 *
 * The returned object is a pointer, and points to a block of memory inside
 * buffer. It however may be distinct from buffer.
 */
OPENVKL_INTERFACE
VKLIntervalIterator vklInitIntervalIterator(VKLVolume volume,
                                            const vkl_vec3f *origin,
                                            const vkl_vec3f *direction,
                                            const vkl_range1f *tRange,
                                            VKLValueSelector valueSelector,
                                            void *buffer);

OPENVKL_INTERFACE
VKLIntervalIterator4 vklInitIntervalIterator4(const int *valid,
                                              VKLVolume volume,
                                              const vkl_vvec3f4 *origin,
                                              const vkl_vvec3f4 *direction,
                                              const vkl_vrange1f4 *tRange,
                                              VKLValueSelector valueSelector,
                                              void *buffer);

OPENVKL_INTERFACE
VKLIntervalIterator8 vklInitIntervalIterator8(const int *valid,
                                              VKLVolume volume,
                                              const vkl_vvec3f8 *origin,
                                              const vkl_vvec3f8 *direction,
                                              const vkl_vrange1f8 *tRange,
                                              VKLValueSelector valueSelector,
                                              void *buffer);

OPENVKL_INTERFACE
VKLIntervalIterator16 vklInitIntervalIterator16(const int *valid,
                                                VKLVolume volume,
                                                const vkl_vvec3f16 *origin,
                                                const vkl_vvec3f16 *direction,
                                                const vkl_vrange1f16 *tRange,
                                                VKLValueSelector valueSelector,
                                                void *buffer);

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

// returns true while the iterator is still within the volume
OPENVKL_INTERFACE
int vklIterateInterval(VKLIntervalIterator iterator, VKLInterval *interval);

OPENVKL_INTERFACE
void vklIterateInterval4(const int *valid,
                         VKLIntervalIterator4 iterator,
                         VKLInterval4 *interval,
                         int *result);

OPENVKL_INTERFACE
void vklIterateInterval8(const int *valid,
                         VKLIntervalIterator8 iterator,
                         VKLInterval8 *interval,
                         int *result);

OPENVKL_INTERFACE
void vklIterateInterval16(const int *valid,
                          VKLIntervalIterator16 iterator,
                          VKLInterval16 *interval,
                          int *result);

///////////////////////////////////////////////////////////////////////////////
// Hit iterators //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct HitIterator;
typedef struct HitIterator *VKLHitIterator;

struct HitIterator4;
typedef struct HitIterator4 *VKLHitIterator4;

struct HitIterator8;
typedef struct HitIterator8 *VKLHitIterator8;

struct HitIterator16;
typedef struct HitIterator16 *VKLHitIterator16;

/*
 * Return the size, in bytes, required to store a hit iterator for the
 * given volumes.
 */
OPENVKL_INTERFACE
size_t vklGetHitIteratorSize(VKLVolume volume);

OPENVKL_INTERFACE
size_t vklGetHitIteratorSize4(VKLVolume volume);

OPENVKL_INTERFACE
size_t vklGetHitIteratorSize8(VKLVolume volume);

OPENVKL_INTERFACE
size_t vklGetHitIteratorSize16(VKLVolume volume);

/*
 * Initialize a hit iterator for the given volume.
 *
 * The buffer must be at leaset vklGetHitIteratorSize*() bytes in size.
 * The returned iterator must not be used after the lifetime of the
 * buffer ends.
 *
 * Users should take care to make buffer allocation as fast as possible.
 * One way to do this is stack allocation using alloca():
 *
 * void *buffer = alloca(vklGetHitIteratorSize(volume);
 * VKLHitIterator it = vklInitializeHitIterator(volume, buffer);
 *
 * If done this way, remember that buffer will be destroyed at the end of the
 * current scope.
 *
 * The returned object is a pointer, and points to a block of memory inside
 * buffer. It however may be distinct from buffer.
 */

OPENVKL_INTERFACE
VKLHitIterator vklInitHitIterator(VKLVolume volume,
                                  const vkl_vec3f *origin,
                                  const vkl_vec3f *direction,
                                  const vkl_range1f *tRange,
                                  VKLValueSelector valueSelector,
                                  void *buffer);

OPENVKL_INTERFACE
VKLHitIterator4 vklInitHitIterator4(const int *valid,
                                    VKLVolume volume,
                                    const vkl_vvec3f4 *origin,
                                    const vkl_vvec3f4 *direction,
                                    const vkl_vrange1f4 *tRange,
                                    VKLValueSelector valueSelector,
                                    void *buffer);

OPENVKL_INTERFACE
VKLHitIterator8 vklInitHitIterator8(const int *valid,
                                    VKLVolume volume,
                                    const vkl_vvec3f8 *origin,
                                    const vkl_vvec3f8 *direction,
                                    const vkl_vrange1f8 *tRange,
                                    VKLValueSelector valueSelector,
                                    void *buffer);

OPENVKL_INTERFACE
VKLHitIterator16 vklInitHitIterator16(const int *valid,
                                      VKLVolume volume,
                                      const vkl_vvec3f16 *origin,
                                      const vkl_vvec3f16 *direction,
                                      const vkl_vrange1f16 *tRange,
                                      VKLValueSelector valueSelector,
                                      void *buffer);


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

// returns true while the iterator is still within the volume
OPENVKL_INTERFACE
int vklIterateHit(VKLHitIterator iterator, VKLHit *hit);

OPENVKL_INTERFACE
void vklIterateHit4(const int *valid,
                    VKLHitIterator4 iterator,
                    VKLHit4 *hit,
                    int *result);

OPENVKL_INTERFACE
void vklIterateHit8(const int *valid,
                    VKLHitIterator8 iterator,
                    VKLHit8 *hit,
                    int *result);

OPENVKL_INTERFACE
void vklIterateHit16(const int *valid,
                     VKLHitIterator16 iterator,
                     VKLHit16 *hit,
                     int *result);

#ifdef __cplusplus
}  // extern "C"
#endif
