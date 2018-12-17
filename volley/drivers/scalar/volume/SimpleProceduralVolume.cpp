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
#include "common/math.h"

namespace volley {
  namespace scalar_driver {

    void SimpleProceduralVolume::commit()
    {
      Volume::commit();
    }

    void SimpleProceduralVolume::intersect(size_t numValues,
                                           const vly_vec3f *origins,
                                           const vly_vec3f *directions,
                                           vly_range1f *ranges)
    {
      // hardcoded bounding box for now
      const box3f boundingBox(vec3f(-1.f), vec3f(1.f));

      // no limits on returned intersections
      range1f rangeLimit(0.f, inf);

      for (size_t i = 0; i < numValues; i++) {
        auto hits = intersectBox(*reinterpret_cast<const vec3f *>(&origins[i]),
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
                                        float *results)
    {
      for (size_t i = 0; i < numValues; i++) {
        results[i] = sinf(worldCoordinates[i].x);
      }
    }

    VLY_REGISTER_VOLUME(SimpleProceduralVolume, simple_procedural_volume)

  }  // namespace scalar_driver
}  // namespace volley
