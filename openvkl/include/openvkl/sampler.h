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
struct VKLSampler : public APIObject
{
};
#else
typedef APIObject VKLSampler;
#endif

#ifdef __cplusplus
extern "C" {
#endif

OPENVKL_INTERFACE VKLSampler vklNewSampler(VKLVolume volume);

#ifdef __cplusplus
}  // extern "C"
#endif
