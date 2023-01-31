// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

OPENVKL_INTERFACE void vklSetBool(APIObject object, const char *name, int b);
OPENVKL_INTERFACE void vklSetFloat(APIObject object, const char *name, float x);
OPENVKL_INTERFACE void vklSetVec3f(
    APIObject object, const char *name, float x, float y, float z);
OPENVKL_INTERFACE void vklSetInt(APIObject object, const char *name, int x);
OPENVKL_INTERFACE void vklSetVec3i(
    APIObject object, const char *name, int x, int y, int z);
OPENVKL_INTERFACE void vklSetData(APIObject object,
                                  const char *name,
                                  VKLData data);
OPENVKL_INTERFACE void vklSetString(APIObject object,
                                    const char *name,
                                    const char *s);
OPENVKL_INTERFACE void vklSetVoidPtr(APIObject object,
                                     const char *name,
                                     void *v);

OPENVKL_INTERFACE void vklSetParam(APIObject object,
                                   const char *name,
                                   VKLDataType dataType,
                                   const void *mem);

#ifdef __cplusplus
}  // extern "C"
#endif
