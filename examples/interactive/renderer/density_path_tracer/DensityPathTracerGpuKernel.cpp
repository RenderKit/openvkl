// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DensityPathTracerGpuKernel.h"

namespace openvkl {
  namespace examples {
    inline vec3f cartesian(const float phi,
                           const float sinTheta,
                           const float cosTheta)
    {
      float sinPhi = sycl::native::sin(phi);
      float cosPhi = sycl::native::cos(phi);
      return vec3f(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta);
    }

    inline vec3f uniformSampleSphere(const float radius, const vec2f &s)
    {
      const float phi      = float(2 * M_PI) * s.x;
      const float cosTheta = radius * (1.f - 2.f * s.y);
      const float sinTheta =
          2.f * radius * sycl::native::sqrt(s.y * (1.f - s.y));
      return cartesian(phi, sinTheta, cosTheta);
    }

    bool DensityPathTracerGpuKernel::sampleWoodcock(RandomTEA &rng,
                                                    const Ray &ray,
                                                    const range1f &hits,
                                                    float &t,
                                                    float &sample,
                                                    float &transmittance) const
    {
      t                    = hits.lower;
      const float sigmaMax = params.sigmaTScale;

      if (sigmaMax <= 0.f) {
        transmittance = 1.f;
        return false;
      }

      while (true) {
        vec2f randomNumbers  = rng.getFloats();
        vec2f randomNumbers2 = rng.getFloats();

        t = t + sycl::native::divide(-sycl::native::log(1.f - randomNumbers.x),
                                     sigmaMax);

        if (t > hits.upper) {
          transmittance = 1.f;
          return false;
        }

        const vec3f c = ray.org + t * ray.dir;
        float time    = rendererParams.time;
        if (params.motionBlur) {
          time = time + (randomNumbers2.x - 0.5f) * params.shutter;
        }
        time = clamp(time, 0.f, 1.f);

        sample = vklComputeSample(&sampler,
                                  (const vkl_vec3f *)&c,
                                  rendererParams.attributeIndex,
                                  time);

        vec4f sampleColorAndOpacity = sampleTransferFunction(sample);

        // sigmaT must be mono-chromatic for Woodcock sampling
        const float sigmaTSample = sigmaMax * sampleColorAndOpacity.w;

        if (randomNumbers.y < sycl::native::divide(sigmaTSample, sigmaMax)) {
          break;
        }
      }

      transmittance = 0.f;
      return true;
    }

    vec3f DensityPathTracerGpuKernel::integrate(RNG &rng,
                                                const Ray *inputRay,
                                                int &maxScatterIndex,
                                                bool &primaryRayIntersect) const
    {
      // Copy ray to local var since we're going to reuse ray object.
      Ray ray = *inputRay;

      vec3f Le(0.f);
      vec3f sigmaSSample(0.f);

      for (int scatterIndex = 0; true; scatterIndex++) {
        float sample    = 0.f;
        maxScatterIndex = max(scatterIndex, maxScatterIndex);

        ray.t = intersectBox(ray.org, ray.dir, volumeBounds);

        // Information needed to render bounding box
        if (scatterIndex == 0 && !ray.t.empty()) {
          primaryRayIntersect = true;
        }

        if (ray.t.empty()) {
          break;
        }

        // where ray meet volumetric particle
        float t = 0.f;

        float transmittance = 0.f;

        const bool haveEvent =
            sampleWoodcock(rng, ray, ray.t, t, sample, transmittance);

        if (!haveEvent) {
          // No hit means that this is end of travel for this ray.
          // For secondary ray we want to get some light from ambient.
          if (scatterIndex > 0) {
            Le += (transmittance * vec3f(params.ambientLightIntensity)) *
                  sigmaSSample;
          }
          break;
        }

        if (scatterIndex >= params.maxNumScatters) {
          break;
        }

        // Compute simgaSSample for this particular hit which can be used in
        // next scattered ray.
        const vec4f sampleColorAndOpacity = sampleTransferFunction(sample);

        sigmaSSample = params.sigmaSScale * vec3f(sampleColorAndOpacity) *
                       sampleColorAndOpacity.w;

        // we need to generate seconday ray (reusing 'ray' object) from where
        // it hit particle (t)
        const vec3f p = ray.org + t * ray.dir;
        ray.t         = range1f(0.f, std::numeric_limits<float>::infinity());
        ray.org       = p;
        ray.dir       = uniformSampleSphere(1.f, rng.getFloats());
      }

      return Le;
    }

    SYCL_EXTERNAL void DensityPathTracerGpuKernel::renderPixel(
        const unsigned int seed,
        const Ray *ray,
        vec4f &rgba,
        float &weight) const

    {
      RNG rng(frameId, seed);
      int maxScatterIndex      = 0;
      bool primaryRayIntersect = false;
      vec3f color = integrate(rng, ray, maxScatterIndex, primaryRayIntersect);

      float alpha = maxScatterIndex > 0 ? 1.f : 0.f;
      if (params.showBbox && primaryRayIntersect) {
        const float bboxBlend = 0.2f;
        alpha                 = (1.f - bboxBlend) * alpha + bboxBlend * 1.f;
        color = (1.f - bboxBlend) * color + bboxBlend * vec3f(1.f);
      }

      rgba.x += color.x;
      rgba.y += color.y;
      rgba.z += color.z;
      rgba.w += alpha;
      weight += 1.f;
    }

    void DensityPathTracerGpuKernel::setObjectAttributes(
        const VKLSampler sampler,
        const box3f volumeBounds,
        const RendererParams &rendererParams,
        const DensityPathTracerParams &params,
        const unsigned int frameId)
    {
      RendererGpuKernel::setObjectAttributes(
          sampler, volumeBounds, rendererParams);
      this->params  = params;
      this->frameId = frameId;
    }

  }  // namespace examples
}  // namespace openvkl
