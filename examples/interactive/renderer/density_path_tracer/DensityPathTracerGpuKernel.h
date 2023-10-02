// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "openvkl_testing.h"

#include "../Random.h"
#include "../Ray.h"
#include "../RendererGpuKernel.h"
#include "../RendererParams.h"
#include "DensityPathTracerParams.h"

namespace openvkl {
  namespace examples {

    class DensityPathTracerGpuKernel final : public RendererGpuKernel
    {
     private:
      DensityPathTracerParams params;
      unsigned int frameId;

      vec3f integrate(RNG &rng,
                      const Ray *inputRay,
                      int &maxScatterIndex,
                      bool &primaryRayIntersect,
                      const VKLFeatureFlags featureFlags) const;

      bool sampleWoodcock(RandomTEA &rng,
                          const Ray &ray,
                          const range1f &hits,
                          float &t,
                          float &sample,
                          float &transmittance,
                          const VKLFeatureFlags featureFlags) const;

     public:
      void setObjectAttributes(const VKLSampler sampler,
                               const box3f volumeBounds,
                               const RendererParams &rendererParams,
                               const DensityPathTracerParams &params,
                               const unsigned int frameId);

      SYCL_EXTERNAL void renderPixel(const unsigned int seed,
                                     const Ray *ray,
                                     vec4f &rgba,
                                     float &weight,
                                     const VKLFeatureFlags featureFlags) const;
    };
  }  // namespace examples
}  // namespace openvkl