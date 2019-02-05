// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#include "volley_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void vlySet1f(VLYObject object, const char *name, float x);
void vlySet3f(VLYObject, const char *name, float x, float y, float z);
void vlySet1i(VLYObject object, const char *name, int x);
void vlySet3i(VLYObject, const char *name, int x, int y, int z);
void vlySetVoidPtr(VLYObject object, const char *name, void *v);

#ifdef __cplusplus
}  // extern "C"
#endif
