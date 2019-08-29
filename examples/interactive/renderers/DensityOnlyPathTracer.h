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

#include "Renderer.h"

namespace openvkl {
  namespace examples {

    struct DirectionalLight
    {
      vec3f power;
      vec3f direction;
      float cosAngle;
      float pdf;
    };

    struct DensityOnlyPathTracer : public Renderer
    {
      DensityOnlyPathTracer()  = default;
      ~DensityOnlyPathTracer() = default;

      void commit() override;

      vec3f renderPixel(VKLVolume volume,
                        const box3f &volumeBounds,
                        VKLSamplesMask mask,
                        Ray &ray) override;

     private:
      bool sampleWoodcock(VKLVolume volume,
                          const Ray &ray,
                          const range1f &hits,
                          float &t,
                          float &sample,
                          float &transmittance);

      void integrateWoodcock(VKLVolume volume,
                             const box3f &volumeBounds,
                             const Ray &ray,
                             vec3f &Le,
                             int scatterIndex);

      // Data //

      float sigmaTScale{0.f};
      float sigmaSScale{0.f};
      int maxNumScatters{0};

      float ambientLightIntensity{0.f};

      DirectionalLight light;
    };

  }  // namespace examples
}  // namespace openvkl