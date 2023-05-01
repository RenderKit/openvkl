// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "openvkl_testing.h"

#include "../Ray.h"
#include "../RendererGpuKernel.h"
#include "../RendererParams.h"
#include "IntervalIteratorDebugParams.h"

namespace openvkl {
  namespace examples {

    class IntervalIteratorDebugGpuKernel final : public RendererGpuKernel
    {
     private:
      IntervalIteratorDebugParams params;

     public:
      void setObjectAttributes(const VKLSampler sampler,
                               const box3f volumeBounds,
                               const RendererParams &rendererParams,
                               const IntervalIteratorDebugParams &params);

      SYCL_EXTERNAL void renderPixel(
          const unsigned int seed,
          const Ray *inputRay,
          vec4f &rgba,
          float &weight,
          void *intervalIteratorBuffer,
          const VKLIntervalIteratorContext intervalContext,
          const VKLFeatureFlags featureFlags) const;
    };
  }  // namespace examples
}  // namespace openvkl