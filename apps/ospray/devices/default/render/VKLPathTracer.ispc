// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "ospray/SDK/common/Model.ih"
#include "ospray/SDK/common/Ray.ih"
#include "ospray/SDK/math/random.ih"
#include "ospray/SDK/math/sampling.ih"
#include "ospray/SDK/render/Renderer.ih"

#include <openvkl/openvkl.isph>

// for very small cones treat as singular light, because float precision is not
// good enough
#define COS_ANGLE_MAX 0.99999988f

struct DirectionalLight
{
  uniform vec3f power;
  uniform vec3f direction;
  uniform float cosAngle;
  uniform float pdf;
};

struct VKLPathTracer
{
  uniform Renderer super;
  uniform VKLVolume volume;
  TransferFunction *uniform transferFunction;

  // if a volume interaction occurs, returns the t and sample value at the
  // location; in this case a scatter event will occur. if a volume interaction
  // does not occur, the ray escapes the volume and the transmittance is
  // returned.
  bool (*uniform sampleVolumeInteraction)(uniform VKLPathTracer *uniform
                                              self,
                                          varying RandomTEA *const uniform rng,
                                          const Ray &ray,
                                          const float tBox0,
                                          const float tBox1,
                                          float &t,
                                          float &sample,
                                          float &transmittance);

  uniform float sigmaTScale;
  uniform float sigmaSScale;
  uniform int maxNumScatters;

  uniform float ambientLightIntensity;

  uniform DirectionalLight directionalLight;
};

struct LightSample
{
  vec3f radiance;  // radiance that arrives at the given point (not weighted by
                   // pdf)
  float pdf;  // probability density that the direction would have been sampled
};

LightSample DirectionalLight_sample(uniform VKLPathTracer *uniform self,
                                    const vec3f &origin,
                                    const vec3f &dir,
                                    const float minDist,
                                    const float maxDist)
{
  const DirectionalLight *uniform light = &self->directionalLight;
  LightSample res;
  res.radiance = make_vec3f(0.f);

  if (inf <= maxDist && light->cosAngle < COS_ANGLE_MAX &&
      dot(light->direction, dir) > light->cosAngle) {
    res.radiance = light->power * light->pdf;
    res.pdf      = light->pdf;
  }

  return res;
}

inline bool sampleWoodcock(uniform VKLPathTracer *uniform self,
                           varying RandomTEA *const uniform rng,
                           const Ray &ray,
                           const float tBox0,
                           const float tBox1,
                           float &t,
                           float &sample,
                           float &transmittance)
{
  t                    = tBox0;
  const float sigmaMax = self->sigmaTScale;

  while (true) {
    vec2f randomNumbers = RandomTEA__getFloats(rng);

    t = t + -logf(1.f - randomNumbers.x) / sigmaMax;

    if (t > tBox1) {
      transmittance = 1.f;
      return false;
    }

    const vec3f c = ray.org + t * ray.dir;
    sample        = vklComputeSampleV(self->volume, (varying vkl_vec3f *)&c);

    const float sampleOpacity = self->transferFunction->getOpacityForValue(
        self->transferFunction, sample);

    // sigmaT must be mono-chromatic for Woodcock sampling
    const float sigmaTSample = sigmaMax * sampleOpacity;

    if (randomNumbers.y < sigmaTSample / sigmaMax) {
      break;
    }
  }

  transmittance = 0.f;
  return true;
}

inline bool sampleRatioTracking(uniform VKLPathTracer *uniform self,
                                varying RandomTEA *const uniform rng,
                                const Ray &ray,
                                const float tBox0,
                                const float tBox1,
                                float &t,
                                float &sample,
                                float &transmittance)
{
  t             = tBox0;
  transmittance = 1.f;

  const float sigmaMax = self->sigmaTScale;

  while (true) {
    vec2f randomNumbers = RandomTEA__getFloats(rng);

    t = t + -logf(1.f - randomNumbers.x) / sigmaMax;

    if (t > tBox1) {
      return false;
    }

    const vec3f c = ray.org + t * ray.dir;
    sample        = vklComputeSampleV(self->volume, (varying vkl_vec3f *)&c);

    const float sampleOpacity = self->transferFunction->getOpacityForValue(
        self->transferFunction, sample);

    // sigmaT must be mono-chromatic for Woodcock sampling
    const float sigmaTSample = sigmaMax * sampleOpacity;

    if (randomNumbers.y < sigmaTSample / sigmaMax) {
      break;
    }

    transmittance *= 1.f - sigmaTSample / sigmaMax;
  }

  transmittance = 0.f;
  return true;
}

inline void integrateWoodcock(uniform VKLPathTracer *uniform self,
                              varying RandomTEA *const uniform rng,
                              const Ray &ray,
                              vec3f &Le,
                              int scatterIndex)
{
  // initialize emitted light to 0
  Le = make_vec3f(0.f);

  const uniform vkl_box3f boundingBox = vklGetBoundingBox(self->volume);

  float tBox0, tBox1;
  const box3f *uniform bb = (const uniform struct box3f *uniform) & boundingBox;
  intersectBox(ray, *bb, tBox0, tBox1);

  if (tBox0 > tBox1)
    return;

  float t, sample, transmittance;

  if (!self->sampleVolumeInteraction(
          self, rng, ray, tBox0, tBox1, t, sample, transmittance)) {
    // light is not directly visible
    if (scatterIndex == 0) {
      return;
    }

    // directional light
    LightSample lightSample =
        DirectionalLight_sample(self, ray.org, ray.dir, tBox1, inf);

    if (reduce_max(lightSample.radiance) > 0.0f)
      Le = Le + transmittance * lightSample.radiance;

    // ambient light
    Le = Le + transmittance * make_vec3f(self->ambientLightIntensity);

    return;
  }

  // new scattering event at sample point
  scatterIndex++;

  if (scatterIndex > self->maxNumScatters) {
    return;
  }

  const vec3f c = ray.org + t * ray.dir;

  const vec3f sampleColor =
      self->transferFunction->getColorForValue(self->transferFunction, sample);

  const float sampleOpacity = self->transferFunction->getOpacityForValue(
      self->transferFunction, sample);

  Ray scatteringRay;
  scatteringRay.t0  = 0.f;
  scatteringRay.t   = inf;
  scatteringRay.org = c;
  scatteringRay.dir = uniformSampleSphere(1.f, RandomTEA__getFloats(rng));

  vec3f inscatteredLe;
  integrateWoodcock(self, rng, scatteringRay, inscatteredLe, scatterIndex + 1);

  const vec3f sigmaSSample = self->sigmaSScale * sampleColor * sampleOpacity;

  Le = Le + sigmaSSample * inscatteredLe;
}

void VKLPathTracer_renderSample(uniform Renderer *uniform _self,
                                   void *uniform perFrameData,
                                   varying ScreenSample &sample)
{
  uniform VKLPathTracer *uniform self =
      (uniform VKLPathTracer * uniform) _self;

  RandomTEA rng_state;
  varying RandomTEA *const uniform rng = &rng_state;
  RandomTEA__Constructor(
      rng,
      sample.sampleID.z,
      (self->super.fb->size.x * sample.sampleID.y) + sample.sampleID.x);

  vec3f Le;
  integrateWoodcock(self, rng, sample.ray, Le, 0);

  sample.rgb   = Le;
  sample.alpha = 1.f;
}

export void *uniform VKLPathTracer_create(void *uniform cppE)
{
  uniform VKLPathTracer *uniform self = uniform new uniform VKLPathTracer;
  Renderer_Constructor(&self->super, cppE, NULL, NULL, 1);
  self->super.renderSample = VKLPathTracer_renderSample;
  return self;
}

export void VKLPathTracer_set(void *uniform _self,
                                 uniform VKLVolume volume,
                                 void *uniform transferFunction,
                                 uniform float sigmaTScale,
                                 uniform float sigmaSScale,
                                 uniform int maxNumScatters,
                                 uniform int useRatioTracking,
                                 uniform float ambientLightIntensity,
                                 uniform float directionalLightIntensity,
                                 uniform float directionalLightAngularDiameter,
                                 const uniform vec3f &directionalLightDirection)
{
  uniform VKLPathTracer *uniform self =
      (uniform VKLPathTracer * uniform) _self;
  self->volume           = volume;
  self->transferFunction = (TransferFunction * uniform) transferFunction;
  self->sigmaTScale      = sigmaTScale;
  self->sigmaSScale      = sigmaSScale;
  self->maxNumScatters   = maxNumScatters;

  if (useRatioTracking) {
    self->sampleVolumeInteraction = sampleRatioTracking;
  } else {
    self->sampleVolumeInteraction = sampleWoodcock;
  }

  self->ambientLightIntensity = ambientLightIntensity;

  self->directionalLight.power = make_vec3f(directionalLightIntensity);

  const uniform float cosAngle = cosf(0.5f * directionalLightAngularDiameter);
  self->directionalLight.cosAngle = cosAngle;
  self->directionalLight.pdf =
      cosAngle < COS_ANGLE_MAX ? uniformSampleConePDF(cosAngle) : inf;

  self->directionalLight.direction = directionalLightDirection;
}
