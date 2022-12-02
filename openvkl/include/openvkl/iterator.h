// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "max_iterator_size.h"
#include "sampler.h"

typedef APIObject VKLIntervalIteratorContext;
typedef APIObject VKLHitIteratorContext;

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// Interval iterators /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
 * Interval iterators require a context.
 */

OPENVKL_INTERFACE
VKLIntervalIteratorContext vklNewIntervalIteratorContext(VKLSampler sampler);

/*
 * Return the size, in bytes, required to store an interval iterator for the
 * given volume.
 */
OPENVKL_INTERFACE
size_t vklGetIntervalIteratorSize(const VKLIntervalIteratorContext *context);

OPENVKL_INTERFACE
size_t vklGetIntervalIteratorSize4(const VKLIntervalIteratorContext *context);

OPENVKL_INTERFACE
size_t vklGetIntervalIteratorSize8(const VKLIntervalIteratorContext *context);

OPENVKL_INTERFACE
size_t vklGetIntervalIteratorSize16(const VKLIntervalIteratorContext *context);

///////////////////////////////////////////////////////////////////////////////
// Hit iterators //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
 * Hit iterators require a context.
 */

OPENVKL_INTERFACE
VKLHitIteratorContext vklNewHitIteratorContext(VKLSampler sampler);

/*
 * Return the size, in bytes, required to store a hit iterator for the
 * given volume.
 */
OPENVKL_INTERFACE
size_t vklGetHitIteratorSize(const VKLHitIteratorContext *context);

OPENVKL_INTERFACE
size_t vklGetHitIteratorSize4(const VKLHitIteratorContext *context);

OPENVKL_INTERFACE
size_t vklGetHitIteratorSize8(const VKLHitIteratorContext *context);

OPENVKL_INTERFACE
size_t vklGetHitIteratorSize16(const VKLHitIteratorContext *context);

#ifdef __cplusplus
}  // extern "C"
#endif
