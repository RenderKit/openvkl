// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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

    vec3f RayMarchIterator::renderPixel(const Scene &scene,
                                        Ray &ray,
                                        const vec4i &sampleID)
    {
      vec3f color(0.f);
      float alpha = 0.f;

      // create volume iterator
      vkl_range1f tRange;
      tRange.lower = ray.t.lower;
      tRange.upper = ray.t.upper;

      void *intervalIteratorBuffer =
          alloca(vklGetIntervalIteratorSize(scene.intervalContext));
      VKLIntervalIterator iterator =
          vklInitIntervalIterator(scene.intervalContext,
                                  (vkl_vec3f *)&ray.org,
                                  (vkl_vec3f *)&ray.dir,
                                  &tRange,
                                  intervalIteratorBuffer);

      // the current ray interval
      VKLInterval interval;

      while (vklIterateInterval(iterator, &interval) && alpha < 0.99f) {
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
          float sample = vklComputeSample(scene.sampler, (vkl_vec3f *)&c);

          // map through transfer function
          vec4f sampleColorAndOpacity = sampleTransferFunction(scene, sample);

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

      return color;
    }

  }  // namespace examples
}  // namespace openvkl
