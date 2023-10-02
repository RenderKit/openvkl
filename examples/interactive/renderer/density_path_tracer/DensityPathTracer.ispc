// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Random.ih"
#include "Ray.ih"
#include "Scene.ih"

#include "openvkl/openvkl.isph"

#include <rkcommon/math/box.ih>
#include <rkcommon/math/vec.ih>

struct DensityPathTracerParams
{
  float shutter;
  bool motionBlur;
  float sigmaTScale;
  float sigmaSScale;
  int maxNumScatters;
  float ambientLightIntensity;
  bool showBbox;
};

export void *uniform DensityPathTracerParams_create()
{
  DensityPathTracerParams *uniform self = uniform new DensityPathTracerParams;
  memset(self, 0, sizeof(uniform DensityPathTracerParams));
  return self;
}

export void DensityPathTracerParams_destroy(void *uniform self)
{
  DensityPathTracerParams *uniform params =
      ((DensityPathTracerParams * uniform) self);
  delete params;
}

export void DensityPathTracerParams_set(void *uniform self,
                                        uniform float shutter,
                                        uniform bool motionBlur,
                                        uniform float sigmaTScale,
                                        uniform float sigmaSScale,
                                        uniform int maxNumScatters,
                                        uniform float ambientLightIntensity,
                                        uniform bool showBbox)
{
  DensityPathTracerParams *uniform params =
      ((DensityPathTracerParams * uniform) self);

  params->shutter               = shutter;
  params->motionBlur            = motionBlur;
  params->sigmaTScale           = sigmaTScale;
  params->sigmaSScale           = sigmaSScale;
  params->maxNumScatters        = maxNumScatters;
  params->ambientLightIntensity = ambientLightIntensity;
  params->showBbox              = showBbox;
}

// -----------------------------------------------------------------------------

bool sampleWoodcock(const uniform DensityPathTracerParams &params,
                    const uniform Scene &scene,
                    RandomTEA &rng,
                    const Ray &ray,
                    float &t,
                    float &sample,
                    float &transmittance)
{
  t                            = ray.tnear;
  const uniform float sigmaMax = params.sigmaTScale;

  if (sigmaMax <= 0.f) {
    transmittance = 1.f;
    return false;
  }

  while (true) {
    vec2f randomNumbers  = RandomTEA__getFloats(&rng);
    vec2f randomNumbers2 = RandomTEA__getFloats(&rng);

    t = t + -logf(1.f - randomNumbers.x) / sigmaMax;

    if (t > ray.tfar) {
      transmittance = 1.f;
      return false;
    }

    const vec3f c = ray.org + t * ray.dir;
    float time    = scene.rendererParams.time;
    if (params.motionBlur) {
      time = time + (randomNumbers2.x - 0.5f) * params.shutter;
    }
    time   = clamp(time, 0.f, 1.f);
    sample = vklComputeSampleV(&scene.sampler,
                               (varying vkl_vec3f * uniform) & c,
                               scene.rendererParams.attributeIndex,
                               &time);

    const vec4f sampleColorAndOpacity =
        TransferFunction_sample(scene.transferFunction, sample);

    // sigmaT must be mono-chromatic for Woodcock sampling
    const float sigmaTSample = sigmaMax * sampleColorAndOpacity.w;

    if (randomNumbers.y < sigmaTSample / sigmaMax) {
      break;
    }
  }

  transmittance = 0.f;
  return true;
}

vec3f integrate(const uniform DensityPathTracerParams &params,
                const uniform Scene &scene,
                RandomTEA &rng,
                const Ray &inputRay,
                int &maxScatterIndex,
                bool &primaryRayIntersect)
{
  // Copy ray to local var since we're going to reuse ray object.
  Ray ray = inputRay;

  vec3f Le           = make_vec3f(0.f);
  vec3f sigmaSSample = make_vec3f(0.f);

  for (uniform int scatterIndex = 0; true; scatterIndex++) {
    float sample    = 0.f;
    maxScatterIndex = max(maxScatterIndex, scatterIndex);

    uniform vkl_box3f volumeBounds = vklGetBoundingBox(scene.volume);
    const box3f *uniform bb =
        (const uniform struct box3f *uniform) & volumeBounds;
    intersectBox(ray, *bb, ray.tnear, ray.tfar);

    // Information needed to render bounding box
    if (scatterIndex == 0 && ray.tnear <= ray.tfar) {
      primaryRayIntersect = true;
    }

    if (!(ray.tnear <= ray.tfar)) {
      break;
    }

    // where ray meet volumetric particle
    float t             = 0.f;
    float transmittance = 0.f;
    const bool haveEvent =
        sampleWoodcock(params, scene, rng, ray, t, sample, transmittance);

    if (!haveEvent) {
      // No hit means that this is end of travel for this ray.
      // For secondary ray we want to get some light from ambient.
      if (scatterIndex > 0) {
        Le = Le + (transmittance * make_vec3f(params.ambientLightIntensity)) *
                      sigmaSSample;
      }
      break;
    }

    if (scatterIndex >= params.maxNumScatters) {
      break;
    }

    // Compute simgaSSample for this particular hit which can be used in
    // next scattered ray.
    const vec4f sampleColorAndOpacity =
        TransferFunction_sample(scene.transferFunction, sample);

    sigmaSSample = params.sigmaSScale * make_vec3f(sampleColorAndOpacity) *
                   sampleColorAndOpacity.w;

    // we need to generate seconday ray (reusing 'ray' object) from where
    // it hit particle (t)
    const vec3f p = ray.org + t * ray.dir;
    ray.tnear     = 0.f;
    ray.tfar      = inf;
    ray.org       = p;
    ray.dir       = uniformSampleSphere(1.f, RandomTEA__getFloats(&rng));
  }
  return Le;
}

// -----------------------------------------------------------------------------

export void DensityPathTracer_renderPixel(const void *uniform _params,
                                          const void *uniform _scene,
                                          uniform uint32 frame,
                                          const uniform vec2i &resolution,
                                          const uniform uint32 offset,
                                          vec4f *uniform rgbas,
                                          float *uniform weights)
{
  const uniform DensityPathTracerParams params =
      *((const DensityPathTracerParams *uniform)_params);
  const uniform Scene &scene = *((const Scene *uniform)_scene);

  const uint32 pixelIdx = offset + programIndex;
  vec2i pixel;
  if (!Scene_computePixel(scene, resolution.x, resolution.y, pixelIdx, pixel)) {
    rgbas[pixelIdx]   = make_vec4f(.18f, .18f, .18f, 1.f);
    weights[pixelIdx] = 1.f;
    return;
  }

  RandomTEA rng;
  RandomTEA__Constructor(&rng, frame, pixelIdx);

  const vec2f pixelf = make_vec2f(pixel.x, pixel.y);
  Ray ray;
  Camera_createRay(scene.camera, resolution, pixelf, ray);

  int maxScatterIndex      = 0;
  bool primaryRayIntersect = false;
  vec3f color =
      integrate(params, scene, rng, ray, maxScatterIndex, primaryRayIntersect);
  float alpha = (maxScatterIndex > 0 ? 1.f : 0.f);

  if (params.showBbox && primaryRayIntersect) {
    const uniform float bboxBlend = 0.2f;
    alpha                         = (1.f - bboxBlend) * alpha + bboxBlend * 1.f;
    color = (1.f - bboxBlend) * color + bboxBlend * make_vec3f(1.f);
  }

  const vec4f rgba =
      rgbas[pixelIdx] + make_vec4f(color.x, color.y, color.z, alpha);
  rgbas[pixelIdx] = rgba;

  const float weight = weights[pixelIdx] + 1.f;
  weights[pixelIdx]  = weight;
}
