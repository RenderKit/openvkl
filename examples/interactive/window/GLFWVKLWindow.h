// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VKLWindow.h"
// glfw
#include <GLFW/glfw3.h>
// std
#include <functional>

namespace openvkl {
  namespace examples {

    using namespace ospcommon::math;

    class GLFWVKLWindow : public VKLWindow
    {
     public:
      GLFWVKLWindow(const vec2i &windowSize,
                    const Scene &scene,
                    std::string rendererType,
                    bool disableVSync);

      ~GLFWVKLWindow() override;

      static GLFWVKLWindow *getActiveWindow();

      void setWindowTitle(const std::string &newWindowTitle);

      void registerImGuiCallback(std::function<void()> callback);
      void registerEndOfFrameCallback(std::function<void()> callback);

      void mainLoop();

     protected:
      void reshape(const vec2i &newWindowSize) override;
      void motion(const vec2f &position);
      void display();

      static GLFWVKLWindow *activeWindow;

      GLFWwindow *glfwWindow = nullptr;

      GLuint framebufferTexture = 0;

      // window title base string (may be added to)
      std::string windowTitle = "OpenVKL";

      bool showUi{true};

      // optional registered ImGui callback, called during every frame to build
      // UI
      std::function<void()> uiCallback;

      // optional callback after a frame was rendered
      std::function<void()> endOfFrameCallback;
    };

  }  // namespace examples
}  // namespace openvkl
