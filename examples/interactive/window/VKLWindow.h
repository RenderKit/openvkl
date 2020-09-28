// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ArcballCamera.h"
#include "renderers/Renderer.h"
// rkcommon
#include "rkcommon/math/box.h"
// std
#include <string>

namespace openvkl {
  namespace examples {

    class VKLWindow
    {
     public:
      VKLWindow(const vec2i &windowSize,
                const Scene& scene,
                std::string rendererType);

      virtual ~VKLWindow() = default;

      void render();

      Renderer &currentRenderer();

      void resetAccumulation();

      void resetCamera();

      void setUseISPC(bool enabled);

      void savePPM(const std::string &filename);

      void setActiveRenderer(const std::string &rendererType);

     protected:
      virtual void reshape(const vec2i &newWindowSize);

      void updateCamera();

      bool useISPC{true};

      vec2i windowSize;

      Renderer *renderer;

      std::unique_ptr<Renderer> renderer_density_pathtracer;
      std::unique_ptr<Renderer> renderer_hit_iterator;
      std::unique_ptr<Renderer> renderer_ray_march_iterator;
      std::unique_ptr<Renderer> renderer_interval_iterator_debug;

      std::unique_ptr<ArcballCamera> arcballCamera;

      const Scene& scene;
    };

  }  // namespace examples
}  // namespace openvkl
