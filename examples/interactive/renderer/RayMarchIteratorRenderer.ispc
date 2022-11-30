// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Ray.ih"
#include "Scene.ih"

#include "openvkl/openvkl.isph"

#include <rkcommon/math/box.ih>
#include <rkcommon/math/vec.ih>

struct RayMarchIteratorRendererParams
{
  float samplingRate;
};

export void *uniform RayMarchIteratorRendererParams_create()
{
  RayMarchIteratorRendererParams *uniform params =
      uniform new RayMarchIteratorRendererParams;
  memset(params, 0, sizeof(uniform RayMarchIteratorRendererParams));
  return params;
}

export void RayMarchIteratorRendererParams_destroy(void *uniform self)
{
  delete ((RayMarchIteratorRendererParams * uniform) self);
}

export void RayMarchIteratorRendererParams_set(void *uniform self,
                                               uniform float samplingRate)
{
  RayMarchIteratorRendererParams *uniform params =
      ((RayMarchIteratorRendererParams * uniform) self);

  params->samplingRate = samplingRate;
}

export void RayMarchIteratorRenderer_renderPixel(
    const void *uniform _params,
    const void *uniform _scene,
    void *uniform _intervalContext,
    const uniform vec2i &resolution,
    const uniform uint32 offset,
    vec4f *uniform rgbas,
    float *uniform weights)
{
  const uniform RayMarchIteratorRendererParams params =
      *((const RayMarchIteratorRendererParams *uniform)_params);
  const uniform Scene &scene = *((const Scene *uniform)_scene);
  VKLIntervalIteratorContext *uniform intervalContext =
      ((VKLIntervalIteratorContext * uniform) _intervalContext);

  const uint32 pixelIdx = offset + programIndex;
  vec2i pixel;
  if (!Scene_computePixel(scene, resolution.x, resolution.y, pixelIdx, pixel)) {
    rgbas[pixelIdx]   = make_vec4f(.18f, .18f, .18f, 1.f);
    weights[pixelIdx] = 1.f;
    return;
  }

  const vec2f pixelf = make_vec2f(pixel.x, pixel.y);
  Ray ray;
  Camera_createRay(scene.camera, resolution, pixelf, ray);

  uniform vkl_box3f volumeBounds = vklGetBoundingBox(scene.volume);
  const box3f *uniform bb =
      (const uniform struct box3f *uniform) & volumeBounds;
  intersectBox(ray, *bb, ray.tnear, ray.tfar);

  vkl_range1f tRange;
  tRange.lower = ray.tnear;
  tRange.upper = ray.tfar;

  const float time = scene.rendererParams.time;

  void *uniform intervalIteratorBuffer =
      alloca(vklGetIntervalIteratorSizeV(intervalContext));

  VKLIntervalIterator iterator =
      vklInitIntervalIteratorV(intervalContext,
                               (varying vkl_vec3f * uniform) & ray.org,
                               (varying vkl_vec3f * uniform) & ray.dir,
                               &tRange,
                               &time,
                               intervalIteratorBuffer);

  VKLInterval interval;

  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  while (vklIterateIntervalV(iterator, &interval) && alpha < 0.99f) {
    const float nominalSamplingDt =
        interval.nominalDeltaT / params.samplingRate;

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
      float sample = vklComputeSampleV(&scene.sampler,
                                       (varying vkl_vec3f * uniform) & c,
                                       scene.rendererParams.attributeIndex,
                                       &time);

      // map through transfer function
      vec4f sampleColorAndOpacity =
          TransferFunction_sample(scene.transferFunction, sample);

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

  rgbas[pixelIdx]   = make_vec4f(color.x, color.y, color.z, 1.f);
  weights[pixelIdx] = 1.f;
}
