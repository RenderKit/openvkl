// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.ih"

struct HitIterator
{
  Renderer super;
};

vec3f HitIterator_renderPixel(Renderer *uniform _self,
                              const uniform Scene* uniform scene,
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

  VKLHitIterator iterator;
  vklInitHitIteratorV(&iterator,
                      scene->volume,
                      (varying vkl_vec3f *)&ray.org,
                      (varying vkl_vec3f *)&ray.dir,
                      &tRange,
                      scene->valueSelector);

  // the current surface hit
  VKLHit hit;

  while (vklIterateHitV(&iterator, &hit) && alpha < 0.99f) {
    vec4f surfaceColorAndOpacity =
        Renderer_sampleTransferFunction(scene, hit.sample);

    color = color + (1.f - alpha) * make_vec3f(surfaceColorAndOpacity);
    alpha = alpha + (1.f - alpha) * 0.25f;
  }

  return color;
}

export void *uniform HitIterator_create()
{
  HitIterator *uniform self = uniform new HitIterator;
  self->super.renderPixel = HitIterator_renderPixel;
  return self;
}
