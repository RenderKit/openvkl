// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "volume.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
struct ValueSelector : public ManagedObject
{
};
#else
typedef ManagedObject ValueSelector;
#endif

typedef ValueSelector *VKLValueSelector;

OPENVKL_INTERFACE VKLValueSelector vklNewValueSelector(VKLVolume volume);

OPENVKL_INTERFACE
void vklValueSelectorSetRanges(VKLValueSelector valueSelector,
                               size_t numRanges,
                               const vkl_range1f *ranges);

OPENVKL_INTERFACE
void vklValueSelectorSetValues(VKLValueSelector valueSelector,
                               size_t numValues,
                               const float *values);

#ifdef __cplusplus
}  // extern "C"
#endif
