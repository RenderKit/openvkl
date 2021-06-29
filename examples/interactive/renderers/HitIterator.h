// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Renderer.h"

namespace openvkl {
  namespace examples {

    struct HitIterator : public Renderer
    {
      HitIterator();
      ~HitIterator() override = default;

      void commit() override;

      vec3f renderPixel(const Scene &scene,
                        Ray &ray,
                        const vec4i &sampleID) override;
    };

  }  // namespace examples
}  // namespace openvkl
