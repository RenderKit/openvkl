// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#include "Renderer.ih"

struct DensityPathTracer
{
  Renderer super;

  float sigmaTScale;
  float sigmaSScale;
  int maxNumScatters;

  float ambientLightIntensity;
};

inline vec3f cartesian(const float phi,
                       const float sinTheta,
                       const float cosTheta)
{
  float sinPhi, cosPhi;
  sincos(phi, &sinPhi, &cosPhi);
  return make_vec3f(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta);
}

inline vec3f uniformSampleSphere(const float radius, const vec2f s)
{
  const float phi      = two_pi * s.x;
  const float cosTheta = radius * (1.f - 2.f * s.y);
  const float sinTheta = 2.f * radius * sqrt(s.y * (1.f - s.y));
  return cartesian(phi, sinTheta, cosTheta);
}

inline bool sampleWoodcock(DensityPathTracer *uniform self,
                           varying RandomTEA *const uniform rng,
                           VKLVolume volume,
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
    sample        = vklComputeSampleV(volume, (varying vkl_vec3f *)&c);

    const float sampleOpacity =
        (sample * (self->super.voxelRange.y - self->super.voxelRange.x)) +
        self->super.voxelRange.x;

    // sigmaT must be mono-chromatic for Woodcock sampling
    const float sigmaTSample = sigmaMax * sampleOpacity;

    if (randomNumbers.y < sigmaTSample / sigmaMax) {
      break;
    }
  }

  transmittance = 0.f;
  return true;
}

inline static void integrate(DensityPathTracer *uniform self,
                             varying RandomTEA *const uniform rng,
                             uniform VKLVolume volume,
                             const uniform box3f &volumeBounds,
                             const Ray &ray,
                             vec3f &Le,
                             int scatterIndex)
{
  // initialize emitted light to 0
  Le = make_vec3f(0.f);

  float tBox0, tBox1;
  const box3f *uniform bb =
      (const uniform struct box3f *uniform) & volumeBounds;
  intersectBox(ray, *bb, tBox0, tBox1);

  if (tBox0 > tBox1)
    return;

  float t, sample, transmittance;

  if (!sampleWoodcock(
          self, rng, volume, ray, tBox0, tBox1, t, sample, transmittance)) {
    // light is not directly visible
    if (scatterIndex == 0) {
      return;
    }

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

  const float sampleOpacity =
      (sample * (self->super.voxelRange.y - self->super.voxelRange.x)) +
      self->super.voxelRange.x;

  Ray scatteringRay;
  scatteringRay.tnear = 0.f;
  scatteringRay.tfar  = inf;
  scatteringRay.org   = c;
  scatteringRay.dir   = uniformSampleSphere(1.f, RandomTEA__getFloats(rng));

  vec3f inscatteredLe;
  integrate(self,
            rng,
            volume,
            volumeBounds,
            scatteringRay,
            inscatteredLe,
            scatterIndex + 1);

  const vec3f sigmaSSample = make_vec3f(self->sigmaSScale * sampleOpacity);

  Le = Le + sigmaSSample * inscatteredLe;
}

vec3f DensityPathTracer_renderPixel(uniform Renderer *uniform _self,
                                    uniform VKLVolume volume,
                                    const uniform box3f &volumeBounds,
                                    uniform VKLSamplesMask,
                                    Ray &ray,
                                    const vec2i &pixel,
                                    const uniform int &frameID,
                                    const uniform int &fbWidth)
{
  DensityPathTracer *uniform self = (DensityPathTracer * uniform) _self;

  RandomTEA rng_state;
  varying RandomTEA *const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, frameID, (fbWidth * pixel.y) + pixel.x);

  vec3f Le;
  integrate(self, rng, volume, volumeBounds, ray, Le, 0);
  return Le;
}

export void *uniform DensityPathTracer_create()
{
  DensityPathTracer *uniform self = uniform new DensityPathTracer;

  self->super.renderPixel = DensityPathTracer_renderPixel;

  return self;
}

export void DensityPathTracer_set(void *uniform _self,
                                  const uniform float sigmaTScale,
                                  const uniform float sigmaSScale,
                                  const uniform int maxNumScatters,
                                  const uniform float ambientLightIntensity)
{
  DensityPathTracer *uniform self = (DensityPathTracer *)_self;

  self->sigmaTScale    = sigmaTScale;
  self->sigmaSScale    = sigmaSScale;
  self->maxNumScatters = maxNumScatters;

  self->ambientLightIntensity = ambientLightIntensity;
}
