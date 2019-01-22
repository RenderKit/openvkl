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

#include "api/Driver.h"

namespace volley {
  namespace scalar_driver {

    struct ScalarDriver : public api::Driver
    {
      ScalarDriver()           = default;
      ~ScalarDriver() override = default;

      virtual void commit() override;

      virtual void commit(VLYObject object) override;

      VLYError loadModule(const char *moduleName) override;

      VLYVolume newVolume(const char *type) override;

      void intersectVolume(VLYVolume volume,
                           size_t numValues,
                           const vly_vec3f *origins,
                           const vly_vec3f *directions,
                           vly_range1f *ranges) override;

      void sampleVolume(VLYVolume volume,
                        VLYSamplingType samplingType,
                        size_t numValues,
                        const vly_vec3f *worldCoordinates,
                        float *results) override;

      void advanceRays(VLYVolume volume,
                       float samplingRate,
                       size_t numValues,
                       const vly_vec3f *origins,
                       const vly_vec3f *directions,
                       float *t) override;

      void integrateVolume(
          VLYVolume volume,
          VLYSamplingType samplingType,
          float samplingRate,
          size_t numValues,
          const vly_vec3f *origins,
          const vly_vec3f *directions,
          const vly_range1f *ranges,
          void *rayUserData,
          IntegrationStepFunction integrationStepFunction) override;
    };

  }  // namespace scalar_driver
}  // namespace volley
