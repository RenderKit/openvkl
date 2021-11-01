// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Ray.ih"
#include "Scene.ih"

#include "openvkl/openvkl.isph"

#include <rkcommon/math/box.ih>
#include <rkcommon/math/vec.ih>

struct IntervalIteratorDebugParams
{
  float intervalColorScale;
  float intervalOpacity;
  bool firstIntervalOnly;
  bool showIntervalBorders;
};

export void *uniform IntervalIteratorDebugParams_create()
{
  IntervalIteratorDebugParams *uniform params =
      uniform new IntervalIteratorDebugParams;
  memset(params, 0, sizeof(uniform IntervalIteratorDebugParams));
  return params;
}

export void IntervalIteratorDebugParams_destroy(void *uniform self)
{
  delete ((IntervalIteratorDebugParams * uniform) self);
}

export void IntervalIteratorDebugParams_set(void *uniform self,
                                            uniform float intervalColorScale,
                                            uniform float intervalOpacity,
                                            uniform bool firstIntervalOnly,
                                            uniform bool showIntervalBorders)
{
  IntervalIteratorDebugParams *uniform params =
      ((IntervalIteratorDebugParams * uniform) self);

  params->intervalColorScale  = intervalColorScale;
  params->intervalOpacity     = intervalOpacity;
  params->firstIntervalOnly   = firstIntervalOnly;
  params->showIntervalBorders = showIntervalBorders;
}

export void IntervalIteratorDebug_renderPixel(const void *uniform _params,
                                              const void *uniform _scene,
                                              void *uniform _intervalContext,
                                              const uniform vec2i &resolution,
                                              const uniform uint32 offset,
                                              vec4f *uniform rgbas,
                                              float *uniform weights)
{
  const uniform IntervalIteratorDebugParams params =
      *((const IntervalIteratorDebugParams *uniform)_params);
  const uniform Scene &scene = *((const Scene *uniform)_scene);
  uniform VKLIntervalIteratorContext intervalContext =
      ((uniform VKLIntervalIteratorContext)_intervalContext);

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

  int intervalCount = 0;
  vec3f color       = make_vec3f(0.f);
  float alpha       = 0.f;

  while (vklIterateIntervalV(iterator, &interval) && alpha < 0.99f) {
    ++intervalCount;

    const float dt = interval.tRange.upper - interval.tRange.lower;

    const float normalizedValue = scene.transferFunction.valueRange.lower +
                                  (float)intervalCount /
                                      params.intervalColorScale *
                                      (scene.transferFunction.valueRange.upper -
                                       scene.transferFunction.valueRange.lower);

    vec4f sampleColorAndOpacity =
        TransferFunction_sample(scene.transferFunction, normalizedValue);

    sampleColorAndOpacity.w = params.intervalOpacity;

    float clampedOpacity = clamp(sampleColorAndOpacity.w * dt);

    sampleColorAndOpacity = sampleColorAndOpacity * clampedOpacity;

    if (params.showIntervalBorders && dt < interval.nominalDeltaT) {
      sampleColorAndOpacity = make_vec4f(1.f);
      clampedOpacity        = 1.f;
    }

    color = color + (1.f - alpha) * make_vec3f(sampleColorAndOpacity);
    alpha = alpha + (1.f - alpha) * clampedOpacity;

    if (params.firstIntervalOnly) {
      break;
    }
  }

  rgbas[pixelIdx]   = make_vec4f(color.x, color.y, color.z, 1.f);
  weights[pixelIdx] = 1.f;
}

