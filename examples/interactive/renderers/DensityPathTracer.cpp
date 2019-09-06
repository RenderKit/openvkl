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

      sigmaTScale    = getParam<float>("sigmaTScale", 1.f);
      sigmaSScale    = getParam<float>("sigmaSScale", 1.f);
      maxNumScatters = getParam<int>("maxNumScatters", 1);

      ambientLightIntensity = getParam<float>("ambientLightIntensity", 1.f);

      ispc::DensityPathTracer_set(ispcEquivalent,
                                  sigmaTScale,
                                  sigmaSScale,
                                  maxNumScatters,
                                  ambientLightIntensity);
    }

    bool DensityPathTracer::sampleWoodcock(RNG &rng,
                                           VKLVolume volume,
                                           const Ray &ray,
                                           const range1f &hits,
                                           float &t,
                                           float &sample,
                                           float &transmittance)
    {
      t = hits.lower;

      const float sigmaMax = sigmaTScale;

      while (true) {
        vec2f randomNumbers = rng.getFloats();

        t = t + -std::log(1.f - randomNumbers.x) / sigmaMax;

        if (t > hits.upper) {
          transmittance = 1.f;
          return false;
        }

        const vec3f c = ray.org + t * ray.dir;
        sample        = vklComputeSample(volume, (const vkl_vec3f *)&c);

        vec4f sampleColorAndOpacity = sampleTransferFunction(sample);

        // sigmaT must be mono-chromatic for Woodcock sampling
        const float sigmaTSample = sigmaMax * sampleColorAndOpacity.w;

        if (randomNumbers.y < sigmaTSample / sigmaMax)
          break;
      }

      transmittance = 0.f;
      return true;
    }

    void DensityPathTracer::integrate(RNG &rng,
                                      VKLVolume volume,
                                      const box3f &volumeBounds,
                                      Ray &ray,
                                      vec3f &Le,
                                      int scatterIndex)
    {
      // initialize emitted light to 0
      Le = vec3f(0.f);

      ray.t = intersectRayBox(ray.org, ray.dir, volumeBounds);
      if (ray.t.empty())
        return;

      float t, sample, transmittance;

      if (!sampleWoodcock(rng, volume, ray, ray.t, t, sample, transmittance)) {
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
      integrate(rng,
                volume,
                volumeBounds,
                scatteringRay,
                inscatteredLe,
                scatterIndex + 1);

      const vec4f sampleColorAndOpacity = sampleTransferFunction(sample);

      const vec3f sigmaSSample =
          sigmaSScale * vec3f(sampleColorAndOpacity) * sampleColorAndOpacity.w;

      Le = Le + sigmaSSample * inscatteredLe;
    }

    vec3f DensityPathTracer::renderPixel(VKLVolume volume,
                                         const box3f &volumeBounds,
                                         VKLSamplesMask,
                                         Ray &ray,
                                         const vec4i &sampleID)
    {
      RNG rng(sampleID.z, (sampleID.w * sampleID.y) + sampleID.x);
      vec3f Le;
      integrate(rng, volume, volumeBounds, ray, Le, 0);
      return Le;
    }

  }  // namespace examples
}  // namespace openvkl
