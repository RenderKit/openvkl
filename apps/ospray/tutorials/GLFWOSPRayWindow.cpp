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

#include "GLFWOSPRayWindow.h"
#include <iostream>
#include <stdexcept>

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"

GLFWOSPRayWindow *GLFWOSPRayWindow::activeWindow = nullptr;

GLFWOSPRayWindow::GLFWOSPRayWindow(const ospcommon::vec2i &windowSize,
                                   const ospcommon::box3f &worldBounds,
                                   OSPModel model,
                                   OSPRenderer renderer)
    : OSPRayWindow(windowSize, worldBounds, model, renderer)
{
  if (activeWindow != nullptr)
    throw std::runtime_error("Cannot create more than one OSPRayWindow!");

  activeWindow = this;

  if (!glfwInit())
    throw std::runtime_error("Failed to initialize GLFW!");

  glfwWindow = glfwCreateWindow(
      windowSize.x, windowSize.y, "OSPRay Tutorial", NULL, NULL);

  if (!glfwWindow) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window!");
  }

  glfwMakeContextCurrent(glfwWindow);

  ImGui_ImplGlfwGL3_Init(glfwWindow, true);

  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);

  glGenTextures(1, &framebufferTexture);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, framebufferTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glfwSetFramebufferSizeCallback(
      glfwWindow, [](GLFWwindow *, int newWidth, int newHeight) {
        activeWindow->reshape(ospcommon::vec2i{newWidth, newHeight});
      });

  glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow *, double x, double y) {
    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureMouse)
      activeWindow->motion(ospcommon::vec2f{float(x), float(y)});
  });

  glfwSetKeyCallback(glfwWindow,
                     [](GLFWwindow *, int key, int, int action, int) {
                       if (action == GLFW_PRESS) {
                         switch (key) {
                         case GLFW_KEY_G:
                           activeWindow->showUi = !(activeWindow->showUi);
                           break;
                         }
                       }
                     });

  // trigger window reshape events with current window size
  glfwGetFramebufferSize(glfwWindow, &this->windowSize.x, &this->windowSize.y);
  reshape(this->windowSize);
}

GLFWOSPRayWindow::~GLFWOSPRayWindow()
{
  ImGui_ImplGlfwGL3_Shutdown();
  glfwTerminate();
}

void GLFWOSPRayWindow::registerImGuiCallback(std::function<void()> callback)
{
  uiCallback = callback;
}

void GLFWOSPRayWindow::mainLoop()
{
  while (!glfwWindowShouldClose(glfwWindow)) {
    ImGui_ImplGlfwGL3_NewFrame();

    display();

    glfwPollEvents();
  }
}

void GLFWOSPRayWindow::reshape(const ospcommon::vec2i &newWindowSize)
{
  OSPRayWindow::reshape(newWindowSize);

  glViewport(0, 0, windowSize.x, windowSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);
}

void GLFWOSPRayWindow::motion(const ospcommon::vec2f &position)
{
  static ospcommon::vec2f previousMouse(-1);

  const ospcommon::vec2f mouse(position.x, position.y);
  if (previousMouse != ospcommon::vec2f(-1)) {
    const bool leftDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool rightDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool middleDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const ospcommon::vec2f prev = previousMouse;

    bool cameraChanged = leftDown || rightDown || middleDown;

    if (leftDown) {
      const ospcommon::vec2f mouseFrom(
          ospcommon::clamp(prev.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
          ospcommon::clamp(prev.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      const ospcommon::vec2f mouseTo(
          ospcommon::clamp(mouse.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
          ospcommon::clamp(mouse.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      arcballCamera->rotate(mouseFrom, mouseTo);
    } else if (rightDown) {
      arcballCamera->zoom(mouse.y - prev.y);
    } else if (middleDown) {
      arcballCamera->pan(ospcommon::vec2f(mouse.x - prev.x, prev.y - mouse.y));
    }

    if (cameraChanged) {
      ospFrameBufferClear(framebuffer, OSP_FB_COLOR | OSP_FB_ACCUM);

      ospSetf(camera, "aspect", windowSize.x / float(windowSize.y));
      ospSetVec3f(camera,
                  "pos",
                  osp::vec3f{arcballCamera->eyePos().x,
                             arcballCamera->eyePos().y,
                             arcballCamera->eyePos().z});
      ospSetVec3f(camera,
                  "dir",
                  osp::vec3f{arcballCamera->lookDir().x,
                             arcballCamera->lookDir().y,
                             arcballCamera->lookDir().z});
      ospSetVec3f(camera,
                  "up",
                  osp::vec3f{arcballCamera->upDir().x,
                             arcballCamera->upDir().y,
                             arcballCamera->upDir().z});

      ospCommit(camera);
    }
  }

  previousMouse = mouse;
}

void GLFWOSPRayWindow::display()
{
  // clock used to compute frame rate
  static auto displayStart = std::chrono::high_resolution_clock::now();

  if (showUi && uiCallback) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin(
        "Tutorial Controls (press 'g' to hide / show)", nullptr, flags);
    uiCallback();
    ImGui::End();
  }

  if (displayCallback) {
    displayCallback(this);
  }

  ospRenderFrame(framebuffer, renderer, OSP_FB_COLOR | OSP_FB_ACCUM);

  uint32_t *fb = (uint32_t *)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);

  glBindTexture(GL_TEXTURE_2D, framebufferTexture);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               windowSize.x,
               windowSize.y,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               fb);

  ospUnmapFrameBuffer(fb, framebuffer);

  glClear(GL_COLOR_BUFFER_BIT);

  glBegin(GL_QUADS);

  glTexCoord2f(0.f, 0.f);
  glVertex2f(0.f, 0.f);

  glTexCoord2f(0.f, 1.f);
  glVertex2f(0.f, windowSize.y);

  glTexCoord2f(1.f, 1.f);
  glVertex2f(windowSize.x, windowSize.y);

  glTexCoord2f(1.f, 0.f);
  glVertex2f(windowSize.x, 0.f);

  glEnd();

  if (showUi && uiCallback) {
    ImGui::Render();
  }

  glfwSwapBuffers(glfwWindow);

  // display frame rate in window title
  auto displayEnd = std::chrono::high_resolution_clock::now();
  auto durationMilliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(displayEnd -
                                                            displayStart);
  displayStart = displayEnd;

  const float frameRate = 1000.f / float(durationMilliseconds.count());

  std::stringstream windowTitle;
  windowTitle << "OSPRay: " << std::setprecision(3) << frameRate << " fps";

  glfwSetWindowTitle(glfwWindow, windowTitle.str().c_str());
}
