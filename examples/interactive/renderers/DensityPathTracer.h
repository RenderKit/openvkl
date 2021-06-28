// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Renderer.h"

namespace openvkl {
  namespace examples {

    struct DensityPathTracer : public Renderer
    {
      DensityPathTracer();
      ~DensityPathTracer() override = default;

      void commit() override;

      vec3f renderPixel(const Scene &scene,
                        Ray &ray,
                        const vec4i &sampleID) override;

     private:
      bool sampleWoodcock(RNG &rng,
                          const Scene &scene,
                          const Ray &ray,
                          const range1f &hits,
                          float &t,
                          float &sample,
                          float &transmittance);

      void integrate(
          RNG &rng, const Scene &scene, Ray &ray, vec3f &Le, int scatterIndex);

      // Data //

      float shutter{0.f};
      bool motionBlur{false};

      float sigmaTScale{0.f};
      float sigmaSScale{0.f};
      int maxNumScatters{0};

      float ambientLightIntensity{0.f};
    };

  }  // namespace examples
}  // namespace openvkl
