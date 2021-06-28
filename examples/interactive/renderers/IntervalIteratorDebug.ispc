// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.ih"

struct IntervalIteratorDebug
{
  Renderer super;
  float intervalColorScale;
  float intervalOpacity;
  bool firstIntervalOnly;
  bool showIntervalBorders;
};

vec3f IntervalIteratorDebug_renderPixel(Renderer *uniform _self,
                                        const uniform Scene *uniform scene,
                                        Ray &ray,
                                        const vec2i &pixel,
                                        const uniform int &frameID,
                                        const uniform int &fbWidth)
{
  IntervalIteratorDebug *uniform self = (IntervalIteratorDebug * uniform) _self;

  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  vkl_range1f tRange;
  tRange.lower = ray.tnear;
  tRange.upper = ray.tfar;

  const float time = scene->time;

  void *uniform intervalIteratorBuffer =
      alloca(vklGetIntervalIteratorSizeV(scene->intervalContext));

  VKLIntervalIterator iterator =
      vklInitIntervalIteratorV(scene->intervalContext,
                               (varying vkl_vec3f * uniform) & ray.org,
                               (varying vkl_vec3f * uniform) & ray.dir,
                               &tRange,
                               &time,
                               intervalIteratorBuffer);

  VKLInterval interval;

  int intervalCount = 0;

  while (vklIterateIntervalV(iterator, &interval) && alpha < 0.99f) {
    intervalCount++;

    const float dt = interval.tRange.upper - interval.tRange.lower;

    const float normalizedValue =
        scene->tfValueRange.lower +
        (float)intervalCount / self->intervalColorScale *
            (scene->tfValueRange.upper - scene->tfValueRange.lower);

    vec4f sampleColorAndOpacity =
        Renderer_sampleTransferFunction(scene, normalizedValue);

    sampleColorAndOpacity.w = self->intervalOpacity;

    float clampedOpacity = clamp(sampleColorAndOpacity.w * dt);

    sampleColorAndOpacity = sampleColorAndOpacity * clampedOpacity;

    if (self->showIntervalBorders && dt < interval.nominalDeltaT) {
      sampleColorAndOpacity = make_vec4f(1.f);
      clampedOpacity        = 1.f;
    }

    color = color + (1.f - alpha) * make_vec3f(sampleColorAndOpacity);
    alpha = alpha + (1.f - alpha) * clampedOpacity;

    if (self->firstIntervalOnly) {
      break;
    }
  }

  return color;
}

export void *uniform IntervalIteratorDebug_create()
{
  IntervalIteratorDebug *uniform self = uniform new IntervalIteratorDebug;
  self->super.renderPixel             = IntervalIteratorDebug_renderPixel;
  self->intervalColorScale            = 4.f;
  self->intervalOpacity               = 0.25f;
  self->firstIntervalOnly             = false;
  self->showIntervalBorders           = false;

  return self;
}

export void *uniform
IntervalIteratorDebug_set(void *uniform _self,
                          const uniform float intervalColorScale,
                          const uniform float intervalOpacity,
                          const uniform bool firstIntervalOnly,
                          const uniform bool showIntervalBorders)
{
  IntervalIteratorDebug *uniform self = (IntervalIteratorDebug * uniform) _self;

  self->intervalColorScale  = intervalColorScale;
  self->intervalOpacity     = intervalOpacity;
  self->firstIntervalOnly   = firstIntervalOnly;
  self->showIntervalBorders = showIntervalBorders;
}
