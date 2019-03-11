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

#include "SharedStructuredVolume_ispc.h"
#include "StructuredVolume.h"

namespace volley {

  namespace ispc_driver {

    struct SharedStructuredVolume : public StructuredVolume
    {
      ~SharedStructuredVolume();

      void commit() override;

      // TODO: single sample through ISPC methods
      float computeSample(const vec3f &objectCoordinates) const override
      {
        return 0.f;
      }

      // TODO: const correctness here
      void computeSample8(const int *valid,
                          const vly_vvec3f8 &objectCoordinates,
                          float *samples) override
      {
        ispc::SharedStructuredVolume_sample_export(
            valid, ispcEquivalent, (void *)&objectCoordinates, (void *)samples);
      }

      // TODO
      vec3f computeGradient(const vec3f &objectCoordinates) const override
      {
        return vec3f(0.f);
      }

     protected:
      std::vector<float> volumeData;

      void *ispcEquivalent{nullptr};
    };

  }  // namespace ispc_driver
}  // namespace volley
