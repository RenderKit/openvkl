// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"

struct Driver;
typedef struct Driver *VKLDriver;

#ifdef __cplusplus
extern "C" {
#endif

OPENVKL_INTERFACE VKLDriver vklNewDriver(const char *driverName);

typedef void (*VKLLogCallback)(void *userData, const char *message);
OPENVKL_INTERFACE void vklDriverSetLogCallback(VKLDriver driver,
                                               VKLLogCallback callback,
                                               void *userData);

typedef void (*VKLErrorCallback)(void *userData,
                                 VKLError error,
                                 const char *message);
OPENVKL_INTERFACE void vklDriverSetErrorCallback(VKLDriver,
                                                 VKLErrorCallback callback,
                                                 void *userData);

OPENVKL_INTERFACE void vklDriverSetInt(VKLDriver driver,
                                       const char *name,
                                       int x);

OPENVKL_INTERFACE void vklDriverSetString(VKLDriver driver,
                                          const char *name,
                                          const char *s);

OPENVKL_INTERFACE void vklCommitDriver(VKLDriver driver);

OPENVKL_INTERFACE void vklSetCurrentDriver(VKLDriver driver);

OPENVKL_INTERFACE VKLDriver vklGetCurrentDriver();

OPENVKL_INTERFACE VKLError vklDriverGetLastErrorCode(VKLDriver driver);

OPENVKL_INTERFACE const char *vklDriverGetLastErrorMsg(VKLDriver driver);

OPENVKL_INTERFACE int vklGetNativeSIMDWidth();

OPENVKL_INTERFACE void vklCommit(VKLObject object);

OPENVKL_INTERFACE void vklRelease(VKLObject object);

OPENVKL_INTERFACE void vklShutdown();

#ifdef __cplusplus
}  // extern "C"
#endif
