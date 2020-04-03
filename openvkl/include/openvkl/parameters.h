// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

OPENVKL_INTERFACE void vklSetBool(VKLObject object, const char *name, int b);
OPENVKL_INTERFACE void vklSetFloat(VKLObject object, const char *name, float x);
OPENVKL_INTERFACE void vklSetVec3f(VKLObject object, const char *name, float x, float y, float z);
OPENVKL_INTERFACE void vklSetInt(VKLObject object, const char *name, int x);
OPENVKL_INTERFACE void vklSetVec3i(VKLObject object, const char *name, int x, int y, int z);
OPENVKL_INTERFACE void vklSetData(VKLObject object, const char *name, VKLData data);
OPENVKL_INTERFACE void vklSetString(VKLObject object, const char *name, const char *s);
OPENVKL_INTERFACE void vklSetVoidPtr(VKLObject object, const char *name, void *v);

#ifdef __cplusplus
}  // extern "C"
#endif
