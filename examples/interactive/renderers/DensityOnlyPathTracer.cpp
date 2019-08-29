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

#include "DensityOnlyPathTracer.h"
// std
#include <random>

namespace openvkl {
  namespace examples {

    static thread_local std::minstd_rand g_rng;
    static std::uniform_real_distribution<float> g_distribution{0.f, 1.f};

// for very small cones treat as singular light, because float precision is not
// good enough
#define COS_ANGLE_MAX 0.99999988f

    struct LightSample
    {
      // radiance that arrives at the given point (not weighted by pdf)
      vec3f radiance{0.f};
      // probability density that the direction would have been sampled
      float pdf{inf};
    };

    static float getRandomUniform()
    {
      return g_distribution(g_rng);
    }

    inline vec3f cartesian(const float phi,
                           const float sinTheta,
                           const float cosTheta)
    {
      float sinPhi = sin(phi);
      float cosPhi = cos(phi);
      return vec3f(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta);
    }

    static vec3f uniformSampleSphere(const float radius, const vec2f &s)
    {
      const float phi      = float(two_pi) * s.x;
      const float cosTheta = radius * (1.f - 2.f * s.y);
      const float sinTheta = 2.f * radius * sqrt(s.y * (1.f - s.y));
      return cartesian(phi, sinTheta, cosTheta);
    }

    static float uniformSampleConePDF(const float cosAngle)
    {
      return rcp(float(two_pi) * (1.0f - cosAngle));
    }

    static LightSample sampleDirectionalLight(const DirectionalLight &light,
                                              const vec3f &origin,
                                              const vec3f &dir)
    {
      LightSample res;

      if (light.cosAngle < COS_ANGLE_MAX &&
          dot(light.direction, dir) > light.cosAngle) {
        res.radiance = light.power * light.pdf;
        res.pdf      = light.pdf;
      }

      return res;
    }

    bool DensityOnlyPathTracer::sampleWoodcock(VKLVolume volume,
                                               const Ray &ray,
                                               const range1f &hits,
                                               float &t,
                                               float &sample,
                                               float &transmittance)
    {
      t = hits.lower;

      const float sigmaMax = sigmaTScale;

      while (true) {
        t = t + -log(1.f - getRandomUniform()) / sigmaMax;

        if (t > hits.upper) {
          transmittance = 1.f;
          return false;
        }

        const vec3f c = ray.org + t * ray.dir;

        sample = vklComputeSample(volume, (const vkl_vec3f *)&c);

        // NOTE(jda) - this should scale based on an input value range
        const float sampleOpacity = sample;

        // sigmaT must be mono-chromatic for Woodcock sampling
        const float sigmaTSample = sigmaMax * sampleOpacity;

        if (getRandomUniform() < sigmaTSample / sigmaMax) {
          break;
        }
      }

      transmittance = 0.f;
      return true;
    }

    void DensityOnlyPathTracer::commit()
    {
      Renderer::commit();

      sigmaTScale    = getParam<float>("sigmaTScale", 1.f);
      sigmaSScale    = getParam<float>("sigmaSScale", 1.f);
      maxNumScatters = getParam<float>("maxNumScatters", 1);

      ambientLightIntensity = getParam<float>("ambientLightIntensity", 1.f);

      light.power = vec3f(getParam<float>("directionalLightIntensity", 1.f));
      float directionalLightAngularDiameter =
          getParam<float>("directionalLightAngularDiameter", 45.f) * M_PI /
          180.f;
      float directionalLightAzimuth =
          getParam<float>("directionalLightAzimuth", 0.f) * M_PI / 180.f;
      float directionalLightElevation =
          getParam<float>("directionalLightElevation", 90.f) * M_PI / 180.f;

      light.direction = vec3f(
          cosf(directionalLightAzimuth) * cosf(directionalLightElevation),
          sinf(directionalLightElevation),
          sinf(directionalLightAzimuth) * cosf(directionalLightElevation));

      const float cosAngle = cosf(0.5f * directionalLightAngularDiameter);

      light.cosAngle = cosAngle;
      light.pdf =
          cosAngle < COS_ANGLE_MAX ? uniformSampleConePDF(cosAngle) : inf;
    }

    void DensityOnlyPathTracer::integrateWoodcock(VKLVolume volume,
                                                  const box3f &volumeBounds,
                                                  const Ray &ray,
                                                  vec3f &Le,
                                                  int scatterIndex)
    {
      // initialize emitted light to 0
      Le = vec3f(0.f);

      const auto hits = intersectRayBox(ray.org, ray.dir, volumeBounds);
      if (hits.empty())
        return;

      float t, sample, transmittance;

      if (!sampleWoodcock(volume, ray, hits, t, sample, transmittance)) {
        if (scatterIndex == 0)
          return;  // light is not directly visible

        // ambient light
        Le += transmittance * vec3f(ambientLightIntensity);

        // directional light
        LightSample lightSample =
            sampleDirectionalLight(light, ray.org, ray.dir);

        if (reduce_max(lightSample.radiance) > 0.0f)
          Le += transmittance * lightSample.radiance;

        return;
      }

      // new scattering event at sample point
      scatterIndex++;

      if (scatterIndex > maxNumScatters)
        return;

      const vec3f c = ray.org + t * ray.dir;

      const vec3f sampleColor(1.f);
      // NOTE(jda) - this should scale based on an input value range
      const float sampleOpacity = sample;

      Ray scatteringRay;
      scatteringRay.tnear = 0.f;
      scatteringRay.tfar  = inf;
      scatteringRay.org   = c;
      scatteringRay.dir   = uniformSampleSphere(
          1.f, vec2f(getRandomUniform(), getRandomUniform()));

      vec3f inscatteredLe;
      integrateWoodcock(
          volume, volumeBounds, scatteringRay, inscatteredLe, scatterIndex + 1);

      const vec3f sigmaSSample = sigmaSScale * sampleColor * sampleOpacity;

      Le = Le + sigmaSSample * inscatteredLe;
    }

    vec3f DensityOnlyPathTracer::renderPixel(VKLVolume volume,
                                             const box3f &volumeBounds,
                                             VKLSamplesMask,
                                             Ray &ray)
    {
      vec3f Le;
      integrateWoodcock(volume, volumeBounds, ray, Le, 0);
      return Le;
    }

  }  // namespace examples
}  // namespace openvkl