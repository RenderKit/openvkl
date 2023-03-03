// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "sampler.h"

#ifdef __cplusplus
struct VKLIntervalIteratorContext : public VKLObject
{
};
struct VKLHitIteratorContext : public VKLObject
{
};
#else
typedef VKLObject VKLIntervalIteratorContext;
typedef VKLObject VKLHitIteratorContext;
#endif

#ifdef __cplusplus
extern "C" {
#endif

NOWARN_C_LINKAGE_PUSH

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

NOWARN_C_LINKAGE_POP

#ifdef __cplusplus
}  // extern "C"
#endif
