// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <cstdlib>
#else
#include <stdint.h>
#include <stdlib.h>
#endif

#include <openvkl/common.h>

typedef APIObject VKLSampler;

#ifdef __cplusplus
extern "C" {
#endif

OPENVKL_INTERFACE
void vklInit();

///////////////////////////////////////////////////////////////////////////////
// Sampling ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// single attribute sampling

OPENVKL_INTERFACE
float vklComputeSample(const VKLSampler *sampler,
                       const vkl_vec3f *objectCoordinates,
                       unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                       float time VKL_DEFAULT_VAL(= 0));

OPENVKL_INTERFACE
void vklComputeSample4(const int *valid,
                       const VKLSampler *sampler,
                       const vkl_vvec3f4 *objectCoordinates,
                       float *samples,
                       unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                       const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSample8(const int *valid,
                       const VKLSampler *sampler,
                       const vkl_vvec3f8 *objectCoordinates,
                       float *samples,
                       unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                       const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSample16(const int *valid,
                        const VKLSampler *sampler,
                        const vkl_vvec3f16 *objectCoordinates,
                        float *samples,
                        unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                        const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSampleN(const VKLSampler *sampler,
                       unsigned int N,
                       const vkl_vec3f *objectCoordinates,
                       float *samples,
                       unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                       const float *times VKL_DEFAULT_VAL(= nullptr));

// multi-attribute sampling

OPENVKL_INTERFACE
void vklComputeSampleM(const VKLSampler *sampler,
                       const vkl_vec3f *objectCoordinates,
                       float *samples,
                       unsigned int M,
                       const unsigned int *attributeIndices,
                       float time VKL_DEFAULT_VAL(= 0));

OPENVKL_INTERFACE
void vklComputeSampleM4(const int *valid,
                        const VKLSampler *sampler,
                        const vkl_vvec3f4 *objectCoordinates,
                        float *samples,
                        unsigned int M,
                        const unsigned int *attributeIndices,
                        const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSampleM8(const int *valid,
                        const VKLSampler *sampler,
                        const vkl_vvec3f8 *objectCoordinates,
                        float *samples,
                        unsigned int M,
                        const unsigned int *attributeIndices,
                        const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSampleM16(const int *valid,
                         const VKLSampler *sampler,
                         const vkl_vvec3f16 *objectCoordinates,
                         float *samples,
                         unsigned int M,
                         const unsigned int *attributeIndices,
                         const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSampleMN(const VKLSampler *sampler,
                        unsigned int N,
                        const vkl_vec3f *objectCoordinates,
                        float *samples,
                        unsigned int M,
                        const unsigned int *attributeIndices,
                        const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
vkl_vec3f vklComputeGradient(const VKLSampler *sampler,
                             const vkl_vec3f *objectCoordinates,
                             unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                             float time VKL_DEFAULT_VAL(= 0));

OPENVKL_INTERFACE
void vklComputeGradient4(const int *valid,
                         const VKLSampler *sampler,
                         const vkl_vvec3f4 *objectCoordinates,
                         vkl_vvec3f4 *gradients,
                         unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                         const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeGradient8(const int *valid,
                         const VKLSampler *sampler,
                         const vkl_vvec3f8 *objectCoordinates,
                         vkl_vvec3f8 *gradients,
                         unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                         const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeGradient16(const int *valid,
                          const VKLSampler *sampler,
                          const vkl_vvec3f16 *objectCoordinates,
                          vkl_vvec3f16 *gradients,
                          unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                          const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeGradientN(const VKLSampler *sampler,
                         unsigned int N,
                         const vkl_vec3f *objectCoordinates,
                         vkl_vec3f *gradients,
                         unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                         const float *times VKL_DEFAULT_VAL(= nullptr));

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
VKLIntervalIterator vklInitIntervalIterator(
    const VKLIntervalIteratorContext *context,
    const vkl_vec3f *origin,
    const vkl_vec3f *direction,
    const vkl_range1f *tRange,
    float time,
    void *buffer);

OPENVKL_INTERFACE
VKLIntervalIterator4 vklInitIntervalIterator4(
    const int *valid,
    const VKLIntervalIteratorContext *context,
    const vkl_vvec3f4 *origin,
    const vkl_vvec3f4 *direction,
    const vkl_vrange1f4 *tRange,
    const float *times,
    void *buffer);

OPENVKL_INTERFACE
VKLIntervalIterator8 vklInitIntervalIterator8(
    const int *valid,
    const VKLIntervalIteratorContext *context,
    const vkl_vvec3f8 *origin,
    const vkl_vvec3f8 *direction,
    const vkl_vrange1f8 *tRange,
    const float *times,
    void *buffer);

OPENVKL_INTERFACE
VKLIntervalIterator16 vklInitIntervalIterator16(
    const int *valid,
    const VKLIntervalIteratorContext *context,
    const vkl_vvec3f16 *origin,
    const vkl_vvec3f16 *direction,
    const vkl_vrange1f16 *tRange,
    const float *times,
    void *buffer);

typedef struct
{
  vkl_range1f tRange;
  vkl_range1f valueRange;
  float nominalDeltaT;
} VKLInterval;

typedef struct VKL_ALIGN(16)
{
  vkl_vrange1f4 tRange;
  vkl_vrange1f4 valueRange;
  float nominalDeltaT[4];
} VKLInterval4;

typedef struct VKL_ALIGN(32)
{
  vkl_vrange1f8 tRange;
  vkl_vrange1f8 valueRange;
  float nominalDeltaT[8];
} VKLInterval8;

typedef struct VKL_ALIGN(64)
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
VKLHitIterator vklInitHitIterator(const VKLHitIteratorContext *context,
                                  const vkl_vec3f *origin,
                                  const vkl_vec3f *direction,
                                  const vkl_range1f *tRange,
                                  float time,
                                  void *buffer);

OPENVKL_INTERFACE
VKLHitIterator4 vklInitHitIterator4(const int *valid,
                                    const VKLHitIteratorContext *context,
                                    const vkl_vvec3f4 *origin,
                                    const vkl_vvec3f4 *direction,
                                    const vkl_vrange1f4 *tRange,
                                    const float *times,
                                    void *buffer);

OPENVKL_INTERFACE
VKLHitIterator8 vklInitHitIterator8(const int *valid,
                                    const VKLHitIteratorContext *context,
                                    const vkl_vvec3f8 *origin,
                                    const vkl_vvec3f8 *direction,
                                    const vkl_vrange1f8 *tRange,
                                    const float *times,
                                    void *buffer);

OPENVKL_INTERFACE
VKLHitIterator16 vklInitHitIterator16(const int *valid,
                                      const VKLHitIteratorContext *context,
                                      const vkl_vvec3f16 *origin,
                                      const vkl_vvec3f16 *direction,
                                      const vkl_vrange1f16 *tRange,
                                      const float *times,
                                      void *buffer);

typedef struct
{
  float t;
  float sample;
  float epsilon;
} VKLHit;

typedef struct VKL_ALIGN(16)
{
  float t[4];
  float sample[4];
  float epsilon[4];
} VKLHit4;

typedef struct VKL_ALIGN(32)
{
  float t[8];
  float sample[8];
  float epsilon[8];
} VKLHit8;

typedef struct VKL_ALIGN(64)
{
  float t[16];
  float sample[16];
  float epsilon[16];
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
