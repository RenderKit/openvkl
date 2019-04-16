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

#include <cstdint>
#include <cstdlib>
#include "common.h"

struct Volume : public ManagedObject
{
};

typedef Volume *VLYVolume;

typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  VLY_SAMPLE_NEAREST = 100,
  VLY_SAMPLE_LINEAR  = 200,
} VLYSamplingMethod;

#ifdef __cplusplus
extern "C" {
#endif

VLYVolume vlyNewVolume(const char *type);

float vlyComputeSample(VLYVolume volume, const vly_vec3f *objectCoordinates);

vly_vec3f vlyComputeGradient(VLYVolume volume,
                             const vly_vec3f *objectCoordinates);

vly_box3f vlyGetBoundingBox(VLYVolume volume);

#ifdef __cplusplus
}  // extern "C"
#endif
