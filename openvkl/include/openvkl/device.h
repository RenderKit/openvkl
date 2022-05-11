// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"

struct VKLDeviceInternal;
typedef struct VKLDeviceInternal *VKLDevice;

#ifdef __cplusplus
extern "C" {
#endif

OPENVKL_INTERFACE VKLDevice vklNewDevice(const char *deviceName);

typedef void (*VKLLogCallback)(void *userData, const char *message);
OPENVKL_INTERFACE void vklDeviceSetLogCallback(VKLDevice device,
                                               VKLLogCallback callback,
                                               void *userData);

typedef void (*VKLErrorCallback)(void *userData,
                                 VKLError error,
                                 const char *message);
OPENVKL_INTERFACE void vklDeviceSetErrorCallback(VKLDevice device,
                                                 VKLErrorCallback callback,
                                                 void *userData);

OPENVKL_INTERFACE void vklDeviceSetInt(VKLDevice device,
                                       const char *name,
                                       int x);

OPENVKL_INTERFACE void vklDeviceSetString(VKLDevice device,
                                          const char *name,
                                          const char *s);

OPENVKL_INTERFACE void vklCommitDevice(VKLDevice device);

OPENVKL_INTERFACE VKLError vklDeviceGetLastErrorCode(VKLDevice device);

OPENVKL_INTERFACE const char *vklDeviceGetLastErrorMsg(VKLDevice device);

OPENVKL_INTERFACE int vklGetNativeSIMDWidth(VKLDevice device);

OPENVKL_INTERFACE void vklCommit(VKLObject object);

OPENVKL_INTERFACE void vklRelease(VKLObject object);

OPENVKL_INTERFACE void vklReleaseDevice(VKLDevice device);

#ifdef __cplusplus
}  // extern "C"
#endif
