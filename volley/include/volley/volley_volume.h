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
#include "volley_common.h"

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
} VLYSamplingType;

#ifdef __cplusplus
extern "C" {
#endif

VLYVolume vlyNewVolume(const char *type);

// returned ranges will be NaN if rays do not intersect
void vlyIntersectVolume(VLYVolume volume,
                        size_t numValues,
                        const vly_vec3f *origins,
                        const vly_vec3f *directions,
                        vly_range1f *ranges);

void vlySampleVolume(VLYVolume volume,
                     VLYSamplingType samplingType,
                     size_t numValues,
                     const vly_vec3f *worldCoordinates,
                     float *results);

// returned t values will be NaN if rays do not intersect or are no longer
// within volume after advancement.
void vlyAdvanceRays(VLYVolume volume,
                    float samplingRate,
                    size_t numValues,
                    const vly_vec3f *origins,
                    const vly_vec3f *directions,
                    float *t);



#ifdef __cplusplus
}  // extern "C"
#endif
