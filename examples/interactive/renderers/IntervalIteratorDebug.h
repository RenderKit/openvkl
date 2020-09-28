// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Renderer.h"

namespace openvkl {
  namespace examples {

    struct IntervalIteratorDebug : public Renderer
    {
      IntervalIteratorDebug();
      ~IntervalIteratorDebug() override = default;

      void commit() override;

      vec3f renderPixel(const Scene &scene,
                        Ray &ray,
                        const vec4i &sampleID) override;

     private:
      float intervalColorScale{4.f};
      float intervalOpacity{0.25f};
      bool firstIntervalOnly{false};
      bool showIntervalBorders{false};
    };

  }  // namespace examples
}  // namespace openvkl
