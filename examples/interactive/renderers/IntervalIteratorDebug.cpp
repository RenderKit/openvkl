// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "IntervalIteratorDebug.h"
// ispc
#include "IntervalIteratorDebug_ispc.h"

namespace openvkl {
  namespace examples {

    IntervalIteratorDebug::IntervalIteratorDebug()
    {
      ispcEquivalent = ispc::IntervalIteratorDebug_create();
    }

    void IntervalIteratorDebug::commit()
    {
      Renderer::commit();

      intervalColorScale  = getParam<float>("intervalColorScale", 4.f);
      intervalOpacity     = getParam<float>("intervalOpacity", 0.25f);
      firstIntervalOnly   = getParam<bool>("firstIntervalOnly", false);
      showIntervalBorders = getParam<bool>("showIntervalBorders", false);

      ispc::IntervalIteratorDebug_set(ispcEquivalent,
                                      intervalColorScale,
                                      intervalOpacity,
                                      firstIntervalOnly,
                                      showIntervalBorders);
    }

    vec3f IntervalIteratorDebug::renderPixel(const Scene &scene,
                                             Ray &ray,
                                             const vec4i &sampleID)
    {
      vec3f color(0.f);
      float alpha = 0.f;

      vkl_range1f tRange;
      tRange.lower = ray.t.lower;
      tRange.upper = ray.t.upper;

      void *intervalIteratorBuffer =
          alloca(vklGetIntervalIteratorSize(scene.sampler));
      VKLIntervalIterator iterator =
          vklInitIntervalIterator(scene.sampler,
                                  (vkl_vec3f *)&ray.org,
                                  (vkl_vec3f *)&ray.dir,
                                  &tRange,
                                  scene.valueSelector,
                                  intervalIteratorBuffer);

      VKLInterval interval;

      int intervalCount = 0;

      while (vklIterateInterval(iterator, &interval) && alpha < 0.99f) {
        intervalCount++;

        const float dt = interval.tRange.upper - interval.tRange.lower;

        const float normalizedValue =
            scene.tfValueRange.lower +
            float(intervalCount) / intervalColorScale *
                (scene.tfValueRange.upper - scene.tfValueRange.lower);

        vec4f sampleColorAndOpacity =
            sampleTransferFunction(scene, normalizedValue);

        sampleColorAndOpacity.w = intervalOpacity;

        float clampedOpacity = clamp(sampleColorAndOpacity.w * dt);

        sampleColorAndOpacity = sampleColorAndOpacity * clampedOpacity;

        if (showIntervalBorders && dt < interval.nominalDeltaT) {
          sampleColorAndOpacity = vec4f(1.f);
          clampedOpacity        = 1.f;
        }

        color = color + (1.f - alpha) * vec3f(sampleColorAndOpacity);
        alpha = alpha + (1.f - alpha) * clampedOpacity;

        if (firstIntervalOnly) {
          break;
        }
      }

      return color;
    }

  }  // namespace examples
}  // namespace openvkl
