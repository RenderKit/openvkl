// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "HitIterator.h"
// ispc
#include "HitIterator_ispc.h"

namespace openvkl {
  namespace examples {

    HitIterator::HitIterator()
    {
      ispcEquivalent = ispc::HitIterator_create();
    }

    vec3f HitIterator::renderPixel(const Scene& scene, Ray &ray, const vec4i &)
    {
      vec3f color(0.f);
      float alpha = 0.f;

      if (scene.valueSelector == nullptr)
        return color;

      // create volume iterator
      vkl_range1f tRange;
      tRange.lower = ray.t.lower;
      tRange.upper = ray.t.upper;

      VKLHitIterator iterator;
      vklInitHitIterator(&iterator,
                         scene.volume,
                         (vkl_vec3f *)&ray.org,
                         (vkl_vec3f *)&ray.dir,
                         &tRange,
                         scene.valueSelector);

      // the current surface hit
      VKLHit hit;

      while (vklIterateHit(&iterator, &hit) && alpha < 0.99f) {
        vec4f surfaceColorAndOpacity = sampleTransferFunction(scene, hit.sample);

        color = color + (1.f - alpha) * vec3f(surfaceColorAndOpacity);
        alpha = alpha + (1.f - alpha) * 0.25f;
      }

      return color;
    }

  }  // namespace examples
}  // namespace openvkl
