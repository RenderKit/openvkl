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

#include "RayMarchIterator.h"
// ispc
#include "RayMarchIterator_ispc.h"

namespace openvkl {
  namespace examples {

    RayMarchIterator::RayMarchIterator()
    {
      ispcEquivalent = ispc::RayMarchIterator_create();
    }

    void RayMarchIterator::commit()
    {
      Renderer::commit();

      samplingRate = getParam<float>("samplingRate", 1.f);

      ispc::RayMarchIterator_set(ispcEquivalent, samplingRate);
    }

    vec3f RayMarchIterator::renderPixel(VKLVolume volume,
                                        const box3f &volumeBounds,
                                        VKLSamplesMask samplesMask,
                                        Ray &ray,
                                        const vec4i &sampleID)
    {
      vec3f color(0.f);
      float alpha = 0.f;

      // create volume iterator
      vkl_range1f tRange;
      tRange.lower = ray.t.lower;
      tRange.upper = ray.t.upper;

      VKLIntervalIterator iterator;
      vklInitIntervalIterator(&iterator,
                              volume,
                              (vkl_vec3f *)&ray.org,
                              (vkl_vec3f *)&ray.dir,
                              &tRange,
                              samplesMask);

      // the current ray interval
      VKLInterval interval;

      while (vklIterateInterval(&iterator, &interval) && alpha < 0.99f) {
        const float nominalSamplingDt = interval.nominalDeltaT / samplingRate;

        // initial sub interval, based on our renderer-defined sampling rate
        // and the volume's nominal dt
        box1f subInterval(interval.tRange.lower,
                          min(interval.tRange.lower + nominalSamplingDt,
                              interval.tRange.upper));

        // integrate as long as we have valid sub intervals and are not
        // fully opaque
        while (subInterval.upper - subInterval.lower > 0.f && alpha < 0.99f) {
          const float t  = 0.5f * (subInterval.lower + subInterval.upper);
          const float dt = subInterval.upper - subInterval.lower;

          // get volume sample
          vec3f c      = ray.org + t * ray.dir;
          float sample = vklComputeSample(volume, (vkl_vec3f *)&c);

          vec3f sampleColor(1.f);

          const float sampleOpacity =
              (sample - voxelRange.lower) / voxelRange.size();

          // accumulate contribution
          const float clampedOpacity = clamp(sampleOpacity * dt);

          sampleColor = sampleColor * clampedOpacity;

          color = color + (1.f - alpha) * sampleColor;
          alpha = alpha + (1.f - alpha) * clampedOpacity;

          // compute next sub interval
          subInterval.lower = subInterval.upper;
          subInterval.upper =
              min(subInterval.lower + nominalSamplingDt, interval.tRange.upper);
        }
      }

      return color;
    }

  }  // namespace examples
}  // namespace openvkl
