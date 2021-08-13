// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.ih"

struct RayMarchIterator
{
  Renderer super;
  float samplingRate;
};

vec3f RayMarchIterator_renderPixel(Renderer *uniform _self,
                                   const uniform Scene* uniform scene,
                                   Ray &ray,
                                   const vec2i &pixel,
                                   const uniform int &frameID,
                                   const uniform int &fbWidth)
{
  RayMarchIterator *uniform self = (RayMarchIterator * uniform) _self;

  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  // create volume iterator
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

  // the current ray interval
  VKLInterval interval;

  while (vklIterateIntervalV(iterator, &interval) && alpha < 0.99f) {
    const float nominalSamplingDt = interval.nominalDeltaT / self->samplingRate;

    // initial sub interval, based on our renderer-defined sampling rate
    // and the volume's nominal dt
    box1f subInterval = make_box1f(
        interval.tRange.lower,
        min(interval.tRange.lower + nominalSamplingDt, interval.tRange.upper));

    // integrate as long as we have valid sub intervals and are not
    // fully opaque
    while (subInterval.upper - subInterval.lower > 0.f && alpha < 0.99f) {
      const float t  = 0.5f * (subInterval.lower + subInterval.upper);
      const float dt = subInterval.upper - subInterval.lower;

      // get volume sample
      vec3f c      = ray.org + t * ray.dir;
      float sample = vklComputeSampleV(scene->sampler,
                                       (varying vkl_vec3f * uniform) & c,
                                       scene->attributeIndex,
                                       &time);

      // map through transfer function
      vec4f sampleColorAndOpacity =
          Renderer_sampleTransferFunction(scene, sample);

      // accumulate contribution
      const float clampedOpacity = clamp(sampleColorAndOpacity.w * dt);

      sampleColorAndOpacity = sampleColorAndOpacity * clampedOpacity;

      color = color + (1.f - alpha) * make_vec3f(sampleColorAndOpacity);
      alpha = alpha + (1.f - alpha) * clampedOpacity;

      // compute next sub interval
      subInterval.lower = subInterval.upper;
      subInterval.upper =
          min(subInterval.lower + nominalSamplingDt, interval.tRange.upper);
    }
  }

  return color;
}

export void *uniform RayMarchIterator_create()
{
  RayMarchIterator *uniform self = uniform new RayMarchIterator;
  self->super.renderPixel = RayMarchIterator_renderPixel;
  self->samplingRate      = 1.f;

  return self;
}

export void *uniform RayMarchIterator_set(void *uniform _self,
                                          const uniform float samplingRate)
{
  RayMarchIterator *uniform self = (RayMarchIterator * uniform) _self;

  self->samplingRate = samplingRate;
}
