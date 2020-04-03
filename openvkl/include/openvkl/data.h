// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "VKLDataType.h"

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
struct Data : public ManagedObject
{
};
#else
typedef ManagedObject Data;
#endif

typedef Data *VKLData;

OPENVKL_INTERFACE VKLData vklNewData(size_t numItems,
                                     VKLDataType dataType,
                                     const void *source,
                                     VKLDataCreationFlags dataCreationFlags
                                         VKL_DEFAULT_VAL(= VKL_DATA_DEFAULT));

#ifdef __cplusplus
}  // extern "C"
#endif
