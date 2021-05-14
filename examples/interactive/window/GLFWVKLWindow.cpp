// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "GLFWVKLWindow.h"
// std
#include <iostream>
#include <stdexcept>
// imgui
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl2.h>
// rkcommon
#include "rkcommon/utility/CodeTimer.h"

namespace openvkl {
  namespace examples {

    GLFWVKLWindow *GLFWVKLWindow::activeWindow = nullptr;

    GLFWVKLWindow::GLFWVKLWindow(const vec2i &windowSize,
                                 const Scene &scene,
                                 std::string rendererType,
                                 bool disableVSync)
        : VKLWindow(windowSize, scene, rendererType)
    {
      if (activeWindow != nullptr)
        throw std::runtime_error("Cannot create more than one VKLWindow!");

      activeWindow = this;

      if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW!");

      glfwWindow = glfwCreateWindow(
          windowSize.x, windowSize.y, "VKL Example", nullptr, nullptr);

      if (!glfwWindow) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window!");
      }

      glfwMakeContextCurrent(glfwWindow);
      if (disableVSync)
        glfwSwapInterval(0);

      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO(); (void)io;
      ImGui::StyleColorsDark();
      ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
      ImGui_ImplOpenGL2_Init();

      glEnable(GL_TEXTURE_2D);
      glDisable(GL_LIGHTING);

      glGenTextures(1, &framebufferTexture);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, framebufferTexture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glfwSetFramebufferSizeCallback(
          glfwWindow, [](GLFWwindow *, int newWidth, int newHeight) {
            activeWindow->reshape(vec2i{newWidth, newHeight});
          });

      glfwSetCursorPosCallback(
          glfwWindow, [](GLFWwindow *, double x, double y) {
            ImGuiIO &io = ImGui::GetIO();
            if (!io.WantCaptureMouse)
              activeWindow->motion(vec2f{float(x), float(y)});
          });

      glfwSetCharCallback(glfwWindow, [](GLFWwindow *, unsigned int c) {
        ImGuiIO &io = ImGui::GetIO();
        if (c > 0 && c < 0x10000)
          io.AddInputCharacter((unsigned short)c);
      });

      glfwSetKeyCallback(
          glfwWindow,
          [](GLFWwindow *gw, int key, int scancode, int action, int mods) {
            ImGui_ImplGlfw_KeyCallback(gw, key, scancode, action, mods);
            if (action == GLFW_PRESS) {
              switch (key) {
              case GLFW_KEY_G:
                activeWindow->showUi = !(activeWindow->showUi);
                break;
              case GLFW_KEY_Q:
                std::exit(0);
                break;
              }
            }
          });

      // trigger window reshape events with current window size
      glfwGetFramebufferSize(
          glfwWindow, &this->windowSize.x, &this->windowSize.y);
      reshape(this->windowSize);
    }

    GLFWVKLWindow::~GLFWVKLWindow()
    {
      ImGui_ImplOpenGL2_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();
      glfwTerminate();
    }

    GLFWVKLWindow *GLFWVKLWindow::getActiveWindow()
    {
      return activeWindow;
    }

    void GLFWVKLWindow::setWindowTitle(const std::string &newWindowTitle)
    {
      windowTitle = newWindowTitle;
    }

    void GLFWVKLWindow::registerImGuiCallback(std::function<void()> callback)
    {
      uiCallback = callback;
    }

    void GLFWVKLWindow::registerEndOfFrameCallback(
        std::function<void()> callback)
    {
      endOfFrameCallback = callback;
    }

    void GLFWVKLWindow::mainLoop()
    {
      while (!glfwWindowShouldClose(glfwWindow)) {
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        display();

        glfwPollEvents();

        if (endOfFrameCallback)
          endOfFrameCallback();
      }
    }

    void GLFWVKLWindow::reshape(const vec2i &newWindowSize)
    {
      VKLWindow::reshape(newWindowSize);

      glViewport(0, 0, windowSize.x, windowSize.y);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);
    }

    void GLFWVKLWindow::motion(const vec2f &position)
    {
      static vec2f previousMouse(-1);

      const vec2f mouse(position.x, position.y);
      if (previousMouse != vec2f(-1)) {
        const bool leftDown =
            glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) ==
            GLFW_PRESS;
        const bool rightDown =
            glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) ==
            GLFW_PRESS;
        const bool middleDown =
            glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) ==
            GLFW_PRESS;
        const vec2f prev = previousMouse;

        bool cameraChanged = leftDown || rightDown || middleDown;

        if (leftDown) {
          const vec2f mouseFrom(
              clamp(prev.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
              clamp(prev.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
          const vec2f mouseTo(
              clamp(mouse.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
              clamp(mouse.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
          arcballCamera->rotate(mouseFrom, mouseTo);
        } else if (rightDown) {
          arcballCamera->zoom(mouse.y - prev.y);
        } else if (middleDown) {
          arcballCamera->pan(vec2f(mouse.x - prev.x, prev.y - mouse.y));
        }

        if (cameraChanged) {
          updateCamera();
        }
      }

      previousMouse = mouse;
    }

    void GLFWVKLWindow::display()
    {
      static rkcommon::utility::CodeTimer displayTimer;
      static rkcommon::utility::CodeTimer renderTimer;

      displayTimer.start();

      if (showUi && uiCallback) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::Begin(
            "Tutorial Controls (press 'g' to hide / show)", nullptr, flags);
        uiCallback();
        ImGui::End();
      }

      renderTimer.start();
      render();
      renderTimer.stop();

      const auto &fb = renderer->frameBuffer();

      glBindTexture(GL_TEXTURE_2D, framebufferTexture);
      glTexImage2D(GL_TEXTURE_2D,
                   0,
                   GL_RGB,
                   windowSize.x,
                   windowSize.y,
                   0,
                   GL_RGB,
                   GL_FLOAT,
                   fb.data());

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
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
      }

      glfwSwapBuffers(glfwWindow);

      displayTimer.stop();

      std::stringstream displayWindowTitle;
      displayWindowTitle << windowTitle << ": " << std::setprecision(3)
                         << displayTimer.perSecond() << " fps | "
                         << std::setprecision(3) << renderTimer.perSecond()
                         << " vkl";

      glfwSetWindowTitle(glfwWindow, displayWindowTitle.str().c_str());
    }

  }  // namespace examples
}  // namespace openvkl
