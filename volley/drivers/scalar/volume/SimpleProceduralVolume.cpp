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

#include "SimpleProceduralVolume.h"
#include <cmath>

namespace volley {
  namespace scalar_driver {

    void SimpleProceduralVolume::commit()
    {
      Volume::commit();

      // hardcoded bounding box for now
      boundingBox = box3f(vec3f(-1.f), vec3f(1.f));

      // nominal voxel size for [(-1, -1, -1), (1, 1, 1)] procedural volume
      // mapping to a resolution of 32x32x32
      voxelSize = 2.f / 32.f;
    }

    void SimpleProceduralVolume::intersect(size_t numValues,
                                           const vly_vec3f *origins,
                                           const vly_vec3f *directions,
                                           vly_range1f *ranges) const
    {
      // no limits on returned intersections
      range1f rangeLimit(0.f, inf);

      for (size_t i = 0; i < numValues; i++) {
        auto hits =
            intersectBox(*reinterpret_cast<const vec3f *>(&origins[i]),
                         *reinterpret_cast<const vec3f *>(&directions[i]),
                         boundingBox,
                         rangeLimit);

        if (hits.first < hits.second && hits.first < rangeLimit.upper) {
          ranges[i].lower = hits.first;
          ranges[i].upper = hits.second;
        } else {
          ranges[i].lower = ranges[i].upper = ospcommon::nan;
        }
      }
    }

    void SimpleProceduralVolume::sample(VLYSamplingType samplingType,
                                        size_t numValues,
                                        const vly_vec3f *worldCoordinates,
                                        float *results) const
    {
      // wavelet parameters
      const float M  = 1.f;
      const float G  = 1.f;
      const float XM = 1.f;
      const float YM = 1.f;
      const float ZM = 1.f;
      const float XF = 3.f;
      const float YF = 3.f;
      const float ZF = 3.f;

      if (samplingType == VLY_SAMPLE_LINEAR) {
        for (size_t i = 0; i < numValues; i++) {
          results[i] = M * G *
                       (XM * sinf(XF * worldCoordinates[i].x) +
                        YM * sinf(YF * worldCoordinates[i].y) +
                        ZM * cosf(ZF * worldCoordinates[i].z));
        }
      } else if (samplingType == VLY_SAMPLE_NEAREST) {
        for (size_t i = 0; i < numValues; i++) {
          // generate modified world coordinates to approximate "nearest"
          // filtering in an actual data-based structured volume
          const vec3i logicalCoordinates =
              (*reinterpret_cast<const vec3f *>(&worldCoordinates[i]) -
               boundingBox.lower) /
              voxelSize;

          const vec3f nearestCoordinates =
              -1.f + logicalCoordinates * voxelSize;

          results[i] = M * G *
                       (XM * sinf(XF * nearestCoordinates.x) +
                        YM * sinf(YF * nearestCoordinates.y) +
                        ZM * cosf(ZF * nearestCoordinates.z));
        }
      } else {
        throw std::runtime_error(
            "sample() called with unimplemented sampling type");
      }
    }

    void SimpleProceduralVolume::advanceRays(float samplingRate,
                                             size_t numValues,
                                             const vly_vec3f *origins,
                                             const vly_vec3f *directions,
                                             float *t) const
    {
      // constant step size within volume, considering sampling rate
      const float step = voxelSize / samplingRate;

      // intersect volume to get feasible t range
      std::vector<vly_range1f> ranges(numValues);
      intersect(numValues, origins, directions, ranges.data());

      for (size_t i = 0; i < numValues; i++) {
        if (ranges[i].lower == (float)ospcommon::nan) {
          // ray does not intersect volume
          t[i] = ospcommon::nan;
        } else if (t[i] < ranges[i].lower) {
          // ray has not yet entered volume; advance to entry point
          t[i] = ranges[i].lower;
        } else if (t[i] + step > ranges[i].upper) {
          // ray has or will have exited the volume
          t[i] = ospcommon::nan;
        } else {
          t[i] += step;
        }
      }
    }

    VLY_REGISTER_VOLUME(SimpleProceduralVolume, simple_procedural_volume)

  }  // namespace scalar_driver
}  // namespace volley
