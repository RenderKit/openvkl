// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "common.h"

struct Driver;
typedef struct Driver *VKLDriver;

#ifdef __cplusplus
extern "C" {
#endif

OPENVKL_INTERFACE VKLDriver vklNewDriver(const char *driverName);

typedef void (*VKLLogFunc)(const char *message);
OPENVKL_INTERFACE void vklDriverSetLogFunc(VKLDriver driver, VKLLogFunc func);

typedef void (*VKLErrorFunc)(VKLError error, const char *message);
OPENVKL_INTERFACE void vklDriverSetErrorFunc(VKLDriver, VKLErrorFunc func);

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
