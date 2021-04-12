// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"

struct Driver;
typedef struct Driver *VKLDriver;

#ifdef __cplusplus
extern "C" {
#endif

OPENVKL_DEPRECATED static inline VKLDriver vklNewDriver(const char *driverName)
{
  return (VKLDriver)vklNewDevice(driverName);
}

OPENVKL_DEPRECATED static inline void vklDriverSetLogCallback(
    VKLDriver driver, VKLLogCallback callback, void *userData)
{
  return vklDeviceSetLogCallback((VKLDevice)driver, callback, userData);
}

OPENVKL_DEPRECATED static inline void vklDriverSetErrorCallback(
    VKLDriver driver, VKLErrorCallback callback, void *userData)
{
  return vklDeviceSetErrorCallback((VKLDevice)driver, callback, userData);
}

OPENVKL_DEPRECATED static inline void vklDriverSetInt(VKLDriver driver,
                                                      const char *name,
                                                      int x)
{
  return vklDeviceSetInt((VKLDevice)driver, name, x);
}

OPENVKL_DEPRECATED static inline void vklDriverSetString(VKLDriver driver,
                                                         const char *name,
                                                         const char *s)
{
  return vklDeviceSetString((VKLDevice)driver, name, s);
}

OPENVKL_DEPRECATED static inline void vklCommitDriver(VKLDriver driver)
{
  return vklCommitDevice((VKLDevice)driver);
}

OPENVKL_DEPRECATED static inline void vklSetCurrentDriver(VKLDriver driver)
{
  return vklSetCurrentDevice((VKLDevice)driver);
}

OPENVKL_DEPRECATED static inline VKLDriver vklGetCurrentDriver()
{
  return (VKLDriver)vklGetCurrentDevice();
}

OPENVKL_DEPRECATED static inline VKLError vklDriverGetLastErrorCode(
    VKLDriver driver)
{
  return vklDeviceGetLastErrorCode((VKLDevice)driver);
}

OPENVKL_DEPRECATED static inline const char *vklDriverGetLastErrorMsg(
    VKLDriver driver)
{
  return vklDeviceGetLastErrorMsg((VKLDevice)driver);
}

#ifdef __cplusplus
}  // extern "C"
#endif
