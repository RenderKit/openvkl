// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.ih"

struct HitIterator
{
  Renderer super;
};

vec3f HitIterator_renderPixel(Renderer *uniform _self,
                              const uniform Scene *uniform scene,
                              Ray &ray,
                              const vec2i &,
                              const uniform int &,
                              const uniform int &)
{
  HitIterator *uniform self = (HitIterator * uniform) _self;

  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  if (scene->valueSelector == NULL)
    return color;

  // create volume iterator
  vkl_range1f tRange;
  tRange.lower = ray.tnear;
  tRange.upper = ray.tfar;

  uniform unsigned int8 hitIteratorBuffer[VKL_MAX_HIT_ITERATOR_SIZE];
  uniform unsigned int8 shadowHitIteratorBuffer[VKL_MAX_HIT_ITERATOR_SIZE];

  VKLHitIterator iterator = vklInitHitIteratorV(scene->volume,
                                                (varying vkl_vec3f *)&ray.org,
                                                (varying vkl_vec3f *)&ray.dir,
                                                &tRange,
                                                scene->valueSelector,
                                                hitIteratorBuffer);

  // the current surface hit
  VKLHit hit;
  const float surfaceAlpha       = 0.6f;
  const uniform float emission[] = {1.f, 0.8f};
  const uniform vec3f lightDir[] = {normalize(make_vec3f(1.f, 1.f, 1.f)),
                                    normalize(make_vec3f(1.f, 1.f, -1.f))};

  while (vklIterateHitV(iterator, &hit) && alpha < 0.99f) {
    const vec3f c = ray.org + hit.t * ray.dir;
    const vkl_vec3f grad =
        vklComputeGradientV(scene->sampler, (varying vkl_vec3f *)&c);
    vec3f N = normalize(make_vec3f(grad.x, grad.y, grad.z));
    if (isnan(N.x) || isnan(N.y) || isnan(N.z))
      N = make_vec3f(0.f);
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
              vklInitHitIteratorV(scene->volume,
                                  (varying vkl_vec3f *)&c,
                                  (varying vkl_vec3f *)&wo,
                                  &tShadowRange,
                                  scene->valueSelector,
                                  shadowHitIteratorBuffer);
          if (!vklIterateHitV(shadowIterator, &shadowHit)) {
            illum += abs(co) * emission[i];  // Lambertian surface shading.
          }
        }
      }
    } else {
      // We have no normal, so do not attempt to shade.
      illum = 1.f;
    }

    // Lambertian surface shading.
    const vec4f surfaceColorAndOpacity =
        Renderer_sampleTransferFunction(scene, hit.sample);
    const vec3f albedo = make_vec3f(surfaceColorAndOpacity);
    color              = color + (1 - alpha) * illum * albedo;
    alpha              = alpha + (1.f - alpha) * surfaceAlpha;
  }

  return color;
}

export void *uniform HitIterator_create()
{
  HitIterator *uniform self = uniform new HitIterator;
  self->super.renderPixel   = HitIterator_renderPixel;
  return self;
}
