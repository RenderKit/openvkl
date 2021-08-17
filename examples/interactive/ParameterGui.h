// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>  // Push/PopDisabled

#include <memory>

namespace openvkl {
  namespace examples {

    class Renderer;
    class RendererParams;
    class Scheduler;
    class Scene;

    class ParameterGui
    {
     public:
      virtual ~ParameterGui();
      virtual bool draw(const Scheduler &scheduler) = 0;

      // Factory methods.
      static std::unique_ptr<ParameterGui> makeRendererGui(Renderer *renderer);
      static std::unique_ptr<ParameterGui> makeRendererParamsGui(Scene *scene);
      static std::unique_ptr<ParameterGui> makeSceneParamsGui(Scene *scene);
    };

  }  // namespace examples
}  // namespace openvkl
