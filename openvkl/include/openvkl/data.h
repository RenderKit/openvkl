// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "VKLDataType.h"
#include "device.h"

// flags that can be passed to vklNewData(), which can be OR'ed together
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  VKL_DATA_DEFAULT       = 0,
  VKL_DATA_SHARED_BUFFER = (1 << 0),
} VKLDataCreationFlags;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
struct VKLData : public APIObject
{
};
#else
typedef APIObject VKLData;
#endif

OPENVKL_INTERFACE VKLData vklNewData(VKLDevice device,
                                     size_t numItems,
                                     VKLDataType dataType,
                                     const void *source,
                                     VKLDataCreationFlags dataCreationFlags
                                         VKL_DEFAULT_VAL(= VKL_DATA_DEFAULT),
                                     size_t byteStride VKL_DEFAULT_VAL(= 0));

#ifdef __cplusplus
}  // extern "C"
#endif
