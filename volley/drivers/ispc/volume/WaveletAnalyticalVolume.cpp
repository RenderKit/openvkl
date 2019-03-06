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

#include "WaveletAnalyticalVolume.h"
#include <cmath>

namespace volley {
  namespace ispc_driver {

    void WaveletAnalyticalVolume::commit()
    {
      Volume::commit();

      // update user-provided parameters
      samplingMethod =
          VLYSamplingMethod(getParam<int>("samplingMethod", VLY_SAMPLE_LINEAR));

      // hardcoded bounding box for now
      boundingBox = box3f(vec3f(-1.f), vec3f(1.f));

      // nominal voxel size for [(-1, -1, -1), (1, 1, 1)] procedural volume
      // mapping to a resolution of 32x32x32
      voxelSize = 2.f / 32.f;
    }

    float WaveletAnalyticalVolume::computeSample(
        const vec3f &objectCoordinates) const
    {
      if (samplingMethod == VLY_SAMPLE_LINEAR) {
        return M * G *
               (XM * sinf(XF * objectCoordinates.x) +
                YM * sinf(YF * objectCoordinates.y) +
                ZM * cosf(ZF * objectCoordinates.z));

      } else if (samplingMethod == VLY_SAMPLE_NEAREST) {
        // generate modified world coordinates to approximate "nearest"
        // filtering in an actual data-based structured volume
        const vec3i logicalCoordinates =
            (objectCoordinates - boundingBox.lower) / voxelSize;

        const vec3f nearestCoordinates = -1.f + logicalCoordinates * voxelSize;

        return M * G *
               (XM * sinf(XF * nearestCoordinates.x) +
                YM * sinf(YF * nearestCoordinates.y) +
                ZM * cosf(ZF * nearestCoordinates.z));
      } else {
        throw std::runtime_error(
            "sample() called with unimplemented sampling type");
      }
    }

    vec3f WaveletAnalyticalVolume::computeGradient(
        const vec3f &objectCoordinates) const
    {
      // TODO: samplingMethod not considered for gradients; do we need it?

      vec3f gradient;

      gradient.x = M * G * (XM * cosf(XF * objectCoordinates.x) * XF);
      gradient.y = M * G * (YM * cosf(YF * objectCoordinates.y) * YF);
      gradient.z = M * G * (ZM * -1.f * sinf(ZF * objectCoordinates.z) * ZF);

      return gradient;
    }

    box3f WaveletAnalyticalVolume::getBoundingBox() const
    {
      return boundingBox;
    }

    VLY_REGISTER_VOLUME(WaveletAnalyticalVolume, wavelet_analytical_volume)

  }  // namespace ispc_driver
}  // namespace volley