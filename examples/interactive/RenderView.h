// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ParameterGui.h"
#include "renderer/framebuffer/Framebuffer.h"

#include <rkcommon/math/vec.h>
#include <cassert>
#include <string>

namespace openvkl {
  namespace examples {

    class Renderer;
    class Scheduler;

    /*
     * An IMGUI window that interacts with a renderer.
     */
    class RenderView
    {
      using vec2f = rkcommon::math::vec2f;
      using vec2i = rkcommon::math::vec2i;

     public:
      explicit RenderView(const std::string &name,
                          std::unique_ptr<Renderer> &&renderer);

      ~RenderView();

      void drawParameterGui();

      // Returns false if not visible.
      bool draw();

      const std::string &getName() const
      {
        return name;
      }

      bool &getVisibleFlag()
      {
        return visible;
      }

      bool isVisible() const
      {
        return static_cast<bool>(renderer) && visible;
      }

      Renderer const &getRenderer() const
      {
        assert(renderer);
        return *(renderer.get());
      }
      Renderer &getRenderer()
      {
        assert(renderer);
        return *(renderer.get());
      }

     private:
      void drawBackground(const ImVec2 &canvasOrigin,
                          const ImVec2 &canvasSize) const;
      void drawFramebuffer(const Scheduler &scheduler,
                           const ImVec2 &canvasOrigin,
                           const ImVec2 &canvasSize) const;
      void drawStats(const Stats &stats,
                     const vec2i &origin) const;

     private:
      std::string name;
      bool visible{true};

      std::string toolboxName;
      bool toolboxVisible{false};

      std::unique_ptr<Renderer> renderer;
      std::unique_ptr<ParameterGui> parameterGui;

      GLuint fbTexId;
    };

  }  // namespace examples
}  // namespace openvkl
