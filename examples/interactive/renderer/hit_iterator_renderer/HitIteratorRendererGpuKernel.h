// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "openvkl_testing.h"

#include "../Ray.h"
#include "../RendererGpuKernel.h"
#include "../RendererParams.h"
#include "HitIteratorRendererGpuKernel.h"
#include "HitIteratorRendererParams.h"

namespace openvkl {
  namespace examples {

    class HitIteratorRendererGpuKernel final : public RendererGpuKernel
    {
     public:
      void setObjectAttributes(const VKLSampler sampler,
                               const box3f volumeBounds,
                               const RendererParams &rendererParams);


      SYCL_EXTERNAL void renderPixel(
          const unsigned int seed,
          const Ray *inputRay,
          vec4f &rgba,
          float &weight,
          void *hitIteratorBuffer,
          void *shadowHitIteratorBuffer,
          const VKLHitIteratorContext hitContext) const;
    };

  }  // namespace examples
}  // namespace openvkl