// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "HitIteratorRendererGpuKernel.h"
#include "rkcommon/math/vec.ih"
namespace openvkl {
  namespace examples {

    SYCL_EXTERNAL void HitIteratorRendererGpuKernel::renderPixel(
        const unsigned int seed,
        const Ray *inputRay,
        vec4f &rgba,
        float &weight,
        void *hitIteratorBuffer,
        void *shadowHitIteratorBuffer,
        const VKLHitIteratorContext hitContext,
        const VKLFeatureFlags featureFlags) const

    {
      Ray ray = *inputRay;
      ray.t   = intersectBox(ray.org, ray.dir, volumeBounds);

      vkl_range1f tRange;
      tRange.lower = ray.t.lower;
      tRange.upper = ray.t.upper;

      VKLHitIterator iterator = vklInitHitIterator(&hitContext,
                                                   (vkl_vec3f *)&ray.org,
                                                   (vkl_vec3f *)&ray.dir,
                                                   &tRange,
                                                   rendererParams.time,
                                                   hitIteratorBuffer,
                                                   featureFlags);

      VKLHit hit;
      const float surfaceAlpha         = 0.6f;
      const float emission[]           = {1.f, 0.8f};
      const ispc::vec3f lightDirISPC[] = {
          ispc::normalize(ispc::make_vec3f(1.f, 1.f, 1.f)),
          ispc::normalize(ispc::make_vec3f(1.f, 1.f, -1.f))};
      const vec3f lightDir[] = {
          vec3f(lightDirISPC[0].x, lightDirISPC[0].y, lightDirISPC[0].z),
          vec3f(lightDirISPC[1].x, lightDirISPC[1].y, lightDirISPC[1].z)};

      vec3f color(0.f);
      float alpha = 0.f;

      while (vklIterateHit(iterator, &hit, featureFlags) && alpha < 0.99f) {
        const vec3f c        = ray.org + hit.t * ray.dir;
        const vkl_vec3f grad = vklComputeGradient(&sampler,
                                                  (vkl_vec3f *)&c,
                                                  rendererParams.attributeIndex,
                                                  rendererParams.time,
                                                  featureFlags);

        const ispc::vec3f N_ispc =
            ispc::normalize(ispc::make_vec3f(grad.x, grad.y, grad.z));
        vec3f N = vec3f(N_ispc.x, N_ispc.y, N_ispc.z);

        if (std::isnan(N.x) || std::isnan(N.y) || std::isnan(N.z)) {
          N = vec3f(0.f);
        }

        // wi and wo both point away from the event.
        const ispc::vec3f wi_ispc = ispc::normalize(ispc::make_vec3f(
            -1.f * ray.dir.x, -1.f * ray.dir.y, -1.f * ray.dir.z));
        const vec3f wi            = vec3f(wi_ispc.x, wi_ispc.y, wi_ispc.z);
        const float ci            = dot(N, wi);

        float illum = 0.f;
        if (length(N) > 0) {
          illum = 0.1f;  // Ambient term.
          // Passing through the surface? Include the surface we started on in
          // alpha.
          for (int i = 0; i < 2; ++i) {
            const vec3f wo = lightDir[i];
            const float co = dot(N, wo);

            // Only test for shadow if we don't have to go through this surface.
            if ((co > 0) == (ci > 0)) {
              VKLHit shadowHit;
              vkl_range1f tShadowRange;
              tShadowRange.lower = hit.epsilon;
              tShadowRange.upper = inf;
              VKLHitIterator shadowIterator =
                  vklInitHitIterator(&hitContext,
                                     (vkl_vec3f *)&c,
                                     (vkl_vec3f *)&wo,
                                     &tShadowRange,
                                     rendererParams.time,
                                     shadowHitIteratorBuffer,
                                     featureFlags);
              if (!vklIterateHit(shadowIterator, &shadowHit, featureFlags)) {
                illum += abs(co) * emission[i];  // Lambertian surface shading.
              }
            }
          }
        } else {
          // We have no normal, so do not attempt to shade.
          illum = 1.f;
        }
        // Lambertian surface shading.
        const vec4f surfaceColorAndOpacity = sampleTransferFunction(hit.sample);
        const vec3f albedo                 = vec3f(surfaceColorAndOpacity);
        color = color + (1 - alpha) * illum * albedo;
        alpha = alpha + (1.f - alpha) * surfaceAlpha;
      }

      rgba.x = color.x;
      rgba.y = color.y;
      rgba.z = color.z;
      rgba.w = 1.f;
      weight = 1.f;
    }

    void HitIteratorRendererGpuKernel::setObjectAttributes(
        const VKLSampler sampler,
        const box3f volumeBounds,
        const RendererParams &rendererParams)
    {
      RendererGpuKernel::setObjectAttributes(
          sampler, volumeBounds, rendererParams);
    }

  }  // namespace examples
}  // namespace openvkl
