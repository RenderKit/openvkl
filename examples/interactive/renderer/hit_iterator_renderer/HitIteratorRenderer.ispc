// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Ray.ih"
#include "Scene.ih"

#include "openvkl/openvkl.isph"

#include <rkcommon/math/box.ih>
#include <rkcommon/math/vec.ih>

struct HitIteratorRendererParams
{
  uint32 numIsoValues;
  const float *isoValues;
};

export void *uniform HitIteratorRendererParams_create()
{
  HitIteratorRendererParams *uniform params =
      uniform new HitIteratorRendererParams;
  memset(params, 0, sizeof(uniform HitIteratorRendererParams));
  return params;
}

export void HitIteratorRendererParams_destroy(void *uniform self)
{
  delete ((HitIteratorRendererParams * uniform) self);
}

export void HitIteratorRendererParams_set(void *uniform self,
                                          uniform uint32 numIsoValues,
                                          const float *uniform isoValues)
{
  HitIteratorRendererParams *uniform params =
      ((HitIteratorRendererParams * uniform) self);

  params->numIsoValues = numIsoValues;
  params->isoValues    = isoValues;
}

// -----------------------------------------------------------------------------

export void HitIteratorRenderer_renderPixel(const void *uniform _params,
                                            const void *uniform _scene,
                                            void *uniform _hitContext,
                                            const uniform vec2i &resolution,
                                            const uniform uint32 offset,
                                            vec4f *uniform rgbas,
                                            float *uniform weights)
{
  const uniform HitIteratorRendererParams params =
      *((const HitIteratorRendererParams *uniform)_params);
  const uniform Scene &scene = *((const Scene *uniform)_scene);
  VKLHitIteratorContext *uniform hitContext =
      ((VKLHitIteratorContext * uniform) _hitContext);

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

  void *uniform hitIteratorBuffer = alloca(vklGetHitIteratorSizeV(hitContext));
  void *uniform shadowHitIteratorBuffer =
      alloca(vklGetHitIteratorSizeV(hitContext));

  VKLHitIterator iterator =
      vklInitHitIteratorV(hitContext,
                          (varying vkl_vec3f * uniform) & ray.org,
                          (varying vkl_vec3f * uniform) & ray.dir,
                          &tRange,
                          &time,
                          hitIteratorBuffer);

  VKLHit hit;
  const float surfaceAlpha = 0.6f;
  const float emission[]   = {1.f, 0.8f};
  const vec3f lightDir[]   = {normalize(make_vec3f(1.f, 1.f, 1.f)),
                            normalize(make_vec3f(1.f, 1.f, -1.f))};

  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  while (vklIterateHitV(iterator, &hit) && alpha < 0.99f) {
    const vec3f c = ray.org + hit.t * ray.dir;
    const vkl_vec3f grad =
        vklComputeGradientV(&scene.sampler,
                            (varying vkl_vec3f * uniform) & c,
                            scene.rendererParams.attributeIndex,
                            &time);
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
              vklInitHitIteratorV(hitContext,
                                  (varying vkl_vec3f * uniform) & c,
                                  (varying vkl_vec3f * uniform) & wo,
                                  &tShadowRange,
                                  &time,
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
        TransferFunction_sample(scene.transferFunction, hit.sample);
    const vec3f albedo = make_vec3f(surfaceColorAndOpacity);
    color              = color + (1 - alpha) * illum * albedo;
    alpha              = alpha + (1.f - alpha) * surfaceAlpha;
  }

  rgbas[pixelIdx]   = make_vec4f(color.x, color.y, color.z, 1.f);
  weights[pixelIdx] = 1.f;
}
