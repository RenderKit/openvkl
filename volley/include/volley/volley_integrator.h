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
#include "volley_volume.h"

struct Integrator : public ManagedObject
{
};

typedef Integrator *VLYIntegrator;

#ifdef __cplusplus
extern "C" {
#endif

VLYIntegrator vlyNewIntegrator(const char *type);

typedef void (*IntegrationStepFunction)(size_t numValues,
                                        const vly_vec3f *worldCoordinates,
                                        const vly_vec3f *directions,
                                        const float *samples,
                                        const vly_vec3f *gradients,
                                        void *rayUserData,
                                        bool *rayTerminationMask);

void vlyIntegrateVolume(VLYIntegrator integrator,
                        VLYVolume volume,
                        size_t numValues,
                        const vly_vec3f *origins,
                        const vly_vec3f *directions,
                        const vly_range1f *ranges,
                        void *rayUserData,
                        IntegrationStepFunction integrationStepFunction);

#ifdef __cplusplus
}  // extern "C"
#endif
