// Copyright 2019-2021 Intel Corporation
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

    void HitIterator::commit()
    {
      Renderer::commit();

      time = getParam<float>("time", 0.f);

      ispc::HitIterator_set(ispcEquivalent, time);
    }

    vec3f HitIterator::renderPixel(const Scene &scene, Ray &ray, const vec4i &)
    {
      vec3f color(0.f);
      float alpha = 0.f;

      if (scene.valueSelector == nullptr)
        return color;

      // create volume iterator
      vkl_range1f tRange;
      tRange.lower = ray.t.lower;
      tRange.upper = ray.t.upper;

      void *hitIteratorBuffer = alloca(vklGetHitIteratorSize(scene.hitContext));
      void *shadowHitIteratorBuffer =
          alloca(vklGetHitIteratorSize(scene.hitContext));

      VKLHitIterator iterator = vklInitHitIterator(scene.hitContext,
                                                   (vkl_vec3f *)&ray.org,
                                                   (vkl_vec3f *)&ray.dir,
                                                   &tRange,
                                                   time,
                                                   scene.valueSelector,
                                                   hitIteratorBuffer);

      // the current surface hit
      VKLHit hit;
      const float surfaceAlpha = 0.6f;
      const float emission[]   = {1.f, 0.8f};
      const vec3f lightDir[]   = {normalize(vec3f(1.f, 1.f, 1.f)),
                                normalize(vec3f(1.f, 1.f, -1.f))};

      while (vklIterateHit(iterator, &hit) && alpha < 0.99f) {
        const vec3f c = ray.org + hit.t * ray.dir;
        const vkl_vec3f grad =
            vklComputeGradient(scene.sampler, (vkl_vec3f *)&c);
        vec3f N = normalize(vec3f(grad.x, grad.y, grad.z));
        if (std::isnan(N.x) || std::isnan(N.y) || std::isnan(N.z))
          N = vec3f(0.f);
        // wi and wo both point away from the event.
        const vec3f wi = normalize(-1.f * ray.dir);
        const float ci = dot(N, wi);

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
                  vklInitHitIterator(scene.hitContext,
                                     (vkl_vec3f *)&c,
                                     (vkl_vec3f *)&wo,
                                     &tShadowRange,
                                     time,
                                     scene.valueSelector,
                                     shadowHitIteratorBuffer);
              if (!vklIterateHit(shadowIterator, &shadowHit)) {
                illum += abs(co) * emission[i];  // Lambertian surface shading.
              }
            }
          }
        } else {
          // We have no normal, so do not attempt to shade.
          illum = 1.f;
        }
        // Lambertian surface shading.
        vec4f surfaceColorAndOpacity =
            sampleTransferFunction(scene, hit.sample);
        const vec3f albedo = vec3f(surfaceColorAndOpacity);
        color              = color + (1 - alpha) * illum * albedo;
        alpha              = alpha + (1.f - alpha) * surfaceAlpha;
      }

      return color;
    }

  }  // namespace examples
}  // namespace openvkl
