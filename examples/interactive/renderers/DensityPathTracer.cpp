// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DensityPathTracer.h"
// ispc
#include "DensityPathTracer_ispc.h"

namespace openvkl {
  namespace examples {

    static vec3f cartesian(const float phi,
                           const float sinTheta,
                           const float cosTheta)
    {
      float sinPhi = std::sin(phi);
      float cosPhi = std::cos(phi);
      return vec3f(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta);
    }

    static vec3f uniformSampleSphere(const float radius, const vec2f &s)
    {
      const float phi      = float(two_pi) * s.x;
      const float cosTheta = radius * (1.f - 2.f * s.y);
      const float sinTheta = 2.f * radius * std::sqrt(s.y * (1.f - s.y));
      return cartesian(phi, sinTheta, cosTheta);
    }

    // DensityPathTracer definitions //////////////////////////////////////////

    DensityPathTracer::DensityPathTracer()
    {
      ispcEquivalent = ispc::DensityPathTracer_create();
    }

    void DensityPathTracer::commit()
    {
      Renderer::commit();

      time                  = getParam<float>("time", 0.f);
      shutter               = getParam<float>("shutter", 0.f);
      motionBlur            = getParam<bool>("motionBlur", false);
      sigmaTScale           = getParam<float>("sigmaTScale", 1.f);
      sigmaSScale           = getParam<float>("sigmaSScale", 1.f);
      maxNumScatters        = getParam<int>("maxNumScatters", 1);
      ambientLightIntensity = getParam<float>("ambientLightIntensity", 1.f);

      ispc::DensityPathTracer_set(ispcEquivalent,
                                  time,
                                  shutter,
                                  motionBlur,
                                  sigmaTScale,
                                  sigmaSScale,
                                  maxNumScatters,
                                  ambientLightIntensity);
    }

    bool DensityPathTracer::sampleWoodcock(RNG &rng,
                                           const Scene &scene,
                                           const Ray &ray,
                                           const range1f &hits,
                                           float &t,
                                           float &sample,
                                           float &transmittance)
    {
      t = hits.lower;

      const float sigmaMax = sigmaTScale;

      while (true) {
        vec2f randomNumbers  = rng.getFloats();
        vec2f randomNumbers2 = rng.getFloats();

        t = t + -std::log(1.f - randomNumbers.x) / sigmaMax;

        if (t > hits.upper) {
          transmittance = 1.f;
          return false;
        }

        const vec3f c = ray.org + t * ray.dir;
        float time    = this->time;
        if (motionBlur) {
          time = time + (randomNumbers2.x - 0.5f) * this->shutter;
        }
        time   = clamp(time, 0.f, 1.f);
        sample = vklComputeSample(
            scene.sampler, (const vkl_vec3f *)&c, scene.attributeIndex, time);

        vec4f sampleColorAndOpacity = sampleTransferFunction(scene, sample);

        // sigmaT must be mono-chromatic for Woodcock sampling
        const float sigmaTSample = sigmaMax * sampleColorAndOpacity.w;

        if (randomNumbers.y < sigmaTSample / sigmaMax)
          break;
      }

      transmittance = 0.f;
      return true;
    }

    void DensityPathTracer::integrate(
        RNG &rng, const Scene &scene, Ray &ray, vec3f &Le, int scatterIndex)
    {
      // initialize emitted light to 0
      Le = vec3f(0.f);

      const auto volumeBounds = vklGetBoundingBox(scene.volume);
      ray.t                   = intersectRayBox(
          ray.org, ray.dir, *reinterpret_cast<const box3f *>(&volumeBounds));
      if (ray.t.empty())
        return;

      float t, sample, transmittance;

      if (!sampleWoodcock(rng, scene, ray, ray.t, t, sample, transmittance)) {
        if (scatterIndex == 0)
          return;  // light is not directly visible

        // ambient light
        Le += transmittance * vec3f(ambientLightIntensity);

        return;
      }

      // new scattering event at sample point
      scatterIndex++;

      if (scatterIndex > maxNumScatters)
        return;

      const vec3f c = ray.org + t * ray.dir;

      Ray scatteringRay;
      scatteringRay.t   = range1f(0.f, inf);
      scatteringRay.org = c;
      scatteringRay.dir = uniformSampleSphere(1.f, rng.getFloats());

      vec3f inscatteredLe;
      integrate(rng, scene, scatteringRay, inscatteredLe, scatterIndex + 1);

      const vec4f sampleColorAndOpacity = sampleTransferFunction(scene, sample);

      const vec3f sigmaSSample =
          sigmaSScale * vec3f(sampleColorAndOpacity) * sampleColorAndOpacity.w;

      Le = Le + sigmaSSample * inscatteredLe;
    }

    vec3f DensityPathTracer::renderPixel(const Scene &scene,
                                         Ray &ray,
                                         const vec4i &sampleID)
    {
      RNG rng(sampleID.z, (sampleID.w * sampleID.y) + sampleID.x);
      vec3f Le;
      integrate(rng, scene, ray, Le, 0);
      return Le;
    }

  }  // namespace examples
}  // namespace openvkl
