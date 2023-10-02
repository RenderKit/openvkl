// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Random.h"
#include "../RendererHost.h"
#include "DensityPathTracerParams.h"

namespace openvkl {
  namespace examples {

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class DensityPathTracer final : public ScalarRenderer
    {
     public:
      DensityPathTracer(Scene &scene);
      ~DensityPathTracer();
      void beforeFrame(bool &needToClear) override final;
      void afterFrame() override final;

      Versioned<DensityPathTracerParams> &getGuiParams() {
        return guiParams;
      }

     protected:
      void renderPixel(size_t seed,
                       Ray &ray,
                       vec4f &rgba,
                       float &weight) const override final;

     private:
      bool sampleWoodcock(RNG &rng,
                          const Ray &ray,
                          const range1f &hits,
                          float &t,
                          float &sample,
                          float &transmittance) const;

      vec3f integrate(RNG &rng,
                      const Ray &inputRay,
                      int scatterIndex,
                      int &maxScatterIndex,
                      bool &primaryRayIntersect) const;

     private:
      Versioned<DensityPathTracerParams> guiParams;
      Versioned<DensityPathTracerParams> params;  // Used by the worker.
      size_t frame{0};  // This is used for random sampling.
    };
  }  // namespace examples
}  // namespace openvkl
