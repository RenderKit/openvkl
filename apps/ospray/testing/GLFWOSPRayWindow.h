// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#include <GLFW/glfw3.h>
#include <functional>
#include "OSPRayWindow.h"

using namespace ospcommon::math;

class GLFWOSPRayWindow : public OSPRayWindow
{
 public:
  GLFWOSPRayWindow(const vec2i &windowSize,
                   const box3f &worldBounds,
                   OSPWorld world,
                   OSPRenderer renderer);

  ~GLFWOSPRayWindow();

  void registerDisplayCallback(
      std::function<void(GLFWOSPRayWindow *)> callback);

  void registerImGuiCallback(std::function<void()> callback);

  void mainLoop();

 protected:
  void reshape(const vec2i &newWindowSize) override;
  void motion(const vec2f &position);
  void display();

  static GLFWOSPRayWindow *activeWindow;

  GLFWwindow *glfwWindow = nullptr;

  GLuint framebufferTexture = 0;

  // toggles display of ImGui UI, if an ImGui callback is provided
  bool showUi = true;

  // optional registered display callback, called before every display()
  std::function<void(GLFWOSPRayWindow *)> displayCallback;

  // optional registered ImGui callback, called during every frame to build UI
  std::function<void()> uiCallback;
};
