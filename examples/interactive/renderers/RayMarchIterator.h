// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Renderer.h"

namespace openvkl {
  namespace examples {

    struct RayMarchIterator : public Renderer
    {
      RayMarchIterator();
      ~RayMarchIterator() override = default;

      void commit() override;

      vec3f renderPixel(const Scene& scene, Ray &ray, const vec4i &sampleID) override;

     private:
      float samplingRate{1.f};
    };

  }  // namespace examples
}  // namespace openvkl
