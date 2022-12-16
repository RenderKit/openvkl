// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "sampler.h"

typedef APIObject VKLIntervalIteratorContext;
typedef APIObject VKLHitIteratorContext;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Interval iterators require a context.
 */
OPENVKL_INTERFACE
VKLIntervalIteratorContext vklNewIntervalIteratorContext(VKLSampler sampler);

/*
 * Hit iterators require a context.
 */
OPENVKL_INTERFACE
VKLHitIteratorContext vklNewHitIteratorContext(VKLSampler sampler);

#ifdef __cplusplus
}  // extern "C"
#endif
