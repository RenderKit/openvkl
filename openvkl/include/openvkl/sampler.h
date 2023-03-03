// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <cstdlib>
#else
#include <stdint.h>
#include <stdlib.h>
#endif

#include "common.h"
#include "volume.h"

#ifdef __cplusplus
struct VKLSampler : public VKLObject
{
};
#else
typedef VKLObject VKLSampler;
#endif

#ifdef __cplusplus
extern "C" {
#endif

NOWARN_C_LINKAGE_PUSH
OPENVKL_INTERFACE VKLSampler vklNewSampler(VKLVolume volume);
NOWARN_C_LINKAGE_POP

#ifdef __cplusplus
}  // extern "C"
#endif
