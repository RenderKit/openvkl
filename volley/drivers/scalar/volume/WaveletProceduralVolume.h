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

#include "Volume.h"
#include "common/math.h"

namespace volley {

  namespace scalar_driver {

    struct WaveletProceduralVolume : public Volume
    {
      void commit() override;

      float sample(const vly_vec3f *objectCoordinates) const override;

      vly_vec3f gradient(const vly_vec3f *objectCoordinates) const override;

      vly_box3f getBoundingBox() const override;

     protected:

      // parameters to be updated on commit
      VLYSamplingMethod samplingMethod = VLY_SAMPLE_LINEAR;
      box3f boundingBox;
      float voxelSize;

      // wavelet parameters
      const float M  = 1.f;
      const float G  = 1.f;
      const float XM = 1.f;
      const float YM = 1.f;
      const float ZM = 1.f;
      const float XF = 3.f;
      const float YF = 3.f;
      const float ZF = 3.f;
    };

  }  // namespace scalar_driver
}  // namespace volley
