// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "IntervalIteratorDebugGpuKernel.h"

namespace openvkl {
  namespace examples {

    SYCL_EXTERNAL void IntervalIteratorDebugGpuKernel::renderPixel(
        const unsigned int seed,
        const Ray *inputRay,
        vec4f &rgba,
        float &weight,
        void *intervalIteratorBuffer,
        const VKLIntervalIteratorContext intervalContext) const

    {
      Ray ray = *inputRay;
      ray.t   = intersectBox(ray.org, ray.dir, volumeBounds);

      vkl_range1f tRange;
      tRange.lower = ray.t.lower;
      tRange.upper = ray.t.upper;

      VKLIntervalIterator iterator =
          vklInitIntervalIterator(&intervalContext,
                                  (vkl_vec3f *)&ray.org,
                                  (vkl_vec3f *)&ray.dir,
                                  &tRange,
                                  rendererParams.time,
                                  intervalIteratorBuffer);

      VKLInterval interval;
      int intervalCount = 0;

      vec3f color(0.f);
      float alpha = 0.f;

      while (vklIterateInterval(iterator, &interval) && alpha < 0.99f) {
        intervalCount++;

        const float dt = interval.tRange.upper - interval.tRange.lower;

        const float normalizedValue =
            rendererParams.transferFunction.valueRange.lower +
            float(intervalCount) / params.intervalColorScale *
                (rendererParams.transferFunction.valueRange.upper -
                 rendererParams.transferFunction.valueRange.lower);

        vec4f sampleColorAndOpacity = sampleTransferFunction(normalizedValue);

        sampleColorAndOpacity.w = params.intervalOpacity;

        float clampedOpacity = clamp(sampleColorAndOpacity.w * dt);

        sampleColorAndOpacity = sampleColorAndOpacity * clampedOpacity;

        if (params.showIntervalBorders && dt < interval.nominalDeltaT) {
          sampleColorAndOpacity = vec4f(1.f);
          clampedOpacity        = 1.f;
        }

        color = color + (1.f - alpha) * vec3f(sampleColorAndOpacity);
        alpha = alpha + (1.f - alpha) * clampedOpacity;

        if (params.firstIntervalOnly) {
          break;
        }
      }

      rgba.x = color.x;
      rgba.y = color.y;
      rgba.z = color.z;
      rgba.w = 1.f;
      weight = 1.f;
    }

    void IntervalIteratorDebugGpuKernel::setObjectAttributes(
        const VKLSampler sampler,
        const box3f volumeBounds,
        const RendererParams &rendererParams,
        const IntervalIteratorDebugParams &params)
    {
      RendererGpuKernel::setObjectAttributes(
          sampler, volumeBounds, rendererParams);
      this->params = params;
    }
  }  // namespace examples
}  // namespace openvkl
