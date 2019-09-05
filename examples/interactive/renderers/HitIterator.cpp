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

#include "HitIterator.h"
// ispc
#include "HitIterator_ispc.h"

namespace openvkl {
  namespace examples {

    HitIterator::HitIterator()
    {
      ispcEquivalent = ispc::HitIterator_create();
    }

    void HitIterator::commit()
    {
      Renderer::commit();

      isovalues    = getParam<const float *>("isovalues", nullptr);
      numIsovalues = getParam<int>("numIsovalues", 0);

      ispc::HitIterator_set(ispcEquivalent, isovalues);
    }

    vec3f HitIterator::renderPixel(VKLVolume volume,
                                   const box3f &volumeBounds,
                                   VKLSamplesMask samplesMask,
                                   Ray &ray,
                                   const vec4i &)
    {
      vec3f color(0.f);
      float alpha = 0.f;

      if (samplesMask == nullptr)
        return color;

      // create volume iterator
      vkl_range1f tRange;
      tRange.lower = ray.t.lower;
      tRange.upper = ray.t.upper;

      VKLHitIterator iterator;
      vklInitHitIterator(&iterator,
                         volume,
                         (vkl_vec3f *)&ray.org,
                         (vkl_vec3f *)&ray.dir,
                         &tRange,
                         samplesMask);

      // the current surface hit
      VKLHit hit;

      while (vklIterateHit(&iterator, &hit) && alpha < 0.99f) {

        vec4f surfaceColorAndOpacity = sampleTransferFunction(hit.sample);

        color = color + (1.f - alpha) * vec3f(surfaceColorAndOpacity);
        alpha = alpha + (1.f - alpha) * 0.25f;
      }

      return color;
    }

  }  // namespace examples
}  // namespace openvkl
