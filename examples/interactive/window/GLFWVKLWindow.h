// ======================================================================== //
// Copyright 2019-2020 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
                    const Scene& scene,
                    std::string rendererType,
                    bool disableVSync);

      ~GLFWVKLWindow() override;

      static GLFWVKLWindow *getActiveWindow();

      void setWindowTitle(const std::string &newWindowTitle);

      void registerImGuiCallback(std::function<void()> callback);

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
    };

  }  // namespace examples
}  // namespace openvkl
