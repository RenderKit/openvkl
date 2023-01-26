// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "sampler.h"

#ifdef __cplusplus
struct VKLIntervalIteratorContext : public APIObject
{
};
struct VKLHitIteratorContext : public APIObject
{
};
#else
typedef APIObject VKLIntervalIteratorContext;
typedef APIObject VKLHitIteratorContext;
#endif

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
