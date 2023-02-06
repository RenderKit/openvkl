// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "RayMarchIteratorGpuKernel.h"

namespace openvkl {
  namespace examples {

    SYCL_EXTERNAL void RayMarchIteratorGpuKernel::renderPixel(
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

      vec3f color(0.f);
      float alpha = 0.f;

      while (vklIterateInterval(iterator, &interval) && alpha < 0.99f) {
        const float nominalSamplingDt =
            interval.nominalDeltaT / params.samplingRate;

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
          vec3f c = ray.org + t * ray.dir;

          float sample = vklComputeSample(&sampler,
                                          (vkl_vec3f *)&c,
                                          rendererParams.attributeIndex,
                                          rendererParams.time);

          // map through transfer function
          vec4f sampleColorAndOpacity = sampleTransferFunction(sample);

          // accumulate contribution
          const float clampedOpacity = clamp(sampleColorAndOpacity.w * dt);

          sampleColorAndOpacity = sampleColorAndOpacity * clampedOpacity;

          color = color + (1.f - alpha) * vec3f(sampleColorAndOpacity);
          alpha = alpha + (1.f - alpha) * clampedOpacity;

          // compute next sub interval
          subInterval.lower = subInterval.upper;
          subInterval.upper =
              min(subInterval.lower + nominalSamplingDt, interval.tRange.upper);
        }
      }

      rgba.x = color.x;
      rgba.y = color.y;
      rgba.z = color.z;
      rgba.w = 1.f;
      weight = 1.f;
    }

    void RayMarchIteratorGpuKernel::setObjectAttributes(
        const VKLSampler sampler,
        const box3f volumeBounds,
        const RendererParams &rendererParams,
        const RayMarchIteratorParams &params)
    {
      RendererGpuKernel::setObjectAttributes(
          sampler, volumeBounds, rendererParams);
      this->params = params;
    }
  }  // namespace examples
}  // namespace openvkl
