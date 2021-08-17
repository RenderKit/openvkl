// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "InteractiveApplication.h"
#include "RenderView.h"
#include "renderer/DensityPathTracer.h"
#include "renderer/Scene.h"

#include <rkcommon/common.h>
#include <rkcommon/math/box.h>
#include <rkcommon/tasking/parallel_for.h>

#include <GLFW/glfw3.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl2.h>
#include <imgui.h>
#include <imgui_internal.h>  // For dockspace API.

#include <iostream>

namespace openvkl {
  namespace examples {

    static void glfwError(int error, const char *description)
    {
      std::cerr << "[EE] " << error << " " << description << std::endl;
    }

    InteractiveApplication::InteractiveApplication()
    {
      glfwSetErrorCallback(glfwError);
      if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW.");
      }
    }

    InteractiveApplication::~InteractiveApplication()
    {
      glfwTerminate();
    }

    void InteractiveApplication::initializeImgui()
    {
      // Setup Dear ImGui context
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO &io = ImGui::GetIO();
      (void)io;
      io.ConfigFlags |=
          ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
      // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable
      // Gamepad Controls
      io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
      io.ConfigFlags |=
          ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport / Platform
                                             // Windows
      io.ConfigViewportsNoAutoMerge   = true;
      io.ConfigViewportsNoTaskBarIcon = true;

      // When dragging inside viewports, we do not want the window to move.
      // Instead, we update the camera.
      io.ConfigWindowsMoveFromTitleBarOnly = true;

      io.ConfigDragClickToInputText = true;

      // Setup Dear ImGui style
      ImGui::StyleColorsDark();
      // ImGui::StyleColorsClassic();

      // When viewports are enabled we tweak WindowRounding/WindowBg so platform
      // windows can look identical to regular ones.
      ImGuiStyle &style = ImGui::GetStyle();
      if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
      }

      // Setup Platform/Renderer backends
      ImGui_ImplGlfw_InitForOpenGL(window, true);
      ImGui_ImplOpenGL2_Init();
    }

    void InteractiveApplication::finalizeImgui()
    {
      ImGui_ImplOpenGL2_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();
    }

    void InteractiveApplication::run(Scene &scene)
    {
      if (!createWindow(scene.disableVSync)) {
        throw std::runtime_error("Failed to create main window.");
      }

      initializeImgui();

      views.clear();
      activeViews.clear();
      inactiveViews.clear();
      scheduler = &scene.scheduler;

      if (scene.rendererTypes.empty()) {
        scene.rendererTypes = {"density_pathtracer_ispc"};
      }

      for (const auto &type : scene.supportedRendererTypes()) {
        auto renderer = scene.createRenderer(type);
        if (!renderer) {
          continue;
        }

        views.emplace_back(
            rkcommon::make_unique<RenderView>(type, std::move(renderer)));

        const bool isVisible           = (std::find(scene.rendererTypes.begin(),
                                          scene.rendererTypes.end(),
                                          type) != scene.rendererTypes.end());
        views.back()->getVisibleFlag() = isVisible;

        if (isVisible) {
          activeViews.push_back(views.back().get());
        } else {
          inactiveViews.push_back(views.back().get());
        }
      }

      const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

      // Note: For initialization, we must build the volumes at least once.
      //       Also, the update code below will launch asynchronous render
      //       threads.
      bool volumeNeedsUpdate = true;

      auto sceneParamsGui = ParameterGui::makeSceneParamsGui(&scene);
      auto rendererParamsGui = ParameterGui::makeRendererParamsGui(&scene);

      while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // When the volume or sampler needs to be updated, we stop all renderers
        // first.  This should happen infrequently. Note that it is much faster
        // to update the sampler than to update the volume.
        if (volumeNeedsUpdate) {
          for (auto *v : activeViews) {
            scheduler->stop(v->getRenderer());
          }

          auto &volume = scene.volume;
          const bool volumeIsDirty = volume.volumeIsDirty();
          volume.updateVKLObjects();
          if (volumeIsDirty) {
            volume.printInfo();
          }
          scheduler->locked(scene.camera, [&]() {
            scene.camera->fitToScreen(volume.getBounds());
            scene.camera.incrementVersion();
          });

          for (auto *v : activeViews) {
            scheduler->start(v->getRenderer());
          }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiID leftNodeId   = 0;
        ImGuiID centerNodeId = 0;
        ImGuiID rightNodeId  = 0;
        initDockspace(leftNodeId, centerNodeId, rightNodeId);

        ImGui::SetNextWindowDockID(leftNodeId, ImGuiCond_FirstUseEver);
        ImGui::Begin("Settings");
        volumeNeedsUpdate = sceneParamsGui->draw(*scheduler);
        ImGui::End();

        ImGui::SetNextWindowDockID(rightNodeId, ImGuiCond_FirstUseEver);
        ImGui::Begin("Renderer Controls");
        {
          rendererParamsGui->draw(*scheduler);

          for (size_t i = 0; i < views.size(); ++i) {
            auto &v = views[i];
            ImGui::PushID(i);
            ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
            if (ImGui::CollapsingHeader(v->getName().c_str(), 0)) {
              ImGui::Checkbox("Enable", &v->getVisibleFlag());
              if (v->isVisible()) {
                v->drawParameterGui();
              }
            }
            ImGui::PopID();
          }
          enableVisibleRenderViews();
        }
        ImGui::End();

        for (auto *v : activeViews) {
          ImGui::SetNextWindowDockID(centerNodeId, ImGuiCond_FirstUseEver);
          v->draw();
        }
        disableInvisibleRenderViews();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w,
                     clear_color.y * clear_color.w,
                     clear_color.z * clear_color.w,
                     clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO &io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
          GLFWwindow *backup_current_context = glfwGetCurrentContext();
          ImGui::UpdatePlatformWindows();
          ImGui::RenderPlatformWindowsDefault();
          glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
      }

      views.clear();
      activeViews.clear();
      inactiveViews.clear();
      scheduler = nullptr;

      finalizeImgui();

      glfwDestroyWindow(window);
      window = nullptr;
    }

    bool InteractiveApplication::createWindow(bool disableVSync)
    {
      window =
          glfwCreateWindow(1920, 1080, "Open VKL Examples", nullptr, nullptr);
      if (!window) {
        return false;
      }
      glfwMakeContextCurrent(window);
      if (disableVSync) {
        glfwSwapInterval(0);
      } else {
        glfwSwapInterval(1);
      }

      return true;
    }

    void InteractiveApplication::initDockspace(unsigned &leftNodeId,
                                               unsigned &centerNodeId,
                                               unsigned &rightNodeId)
    {
      // The dock space is a window, but we do not want it to show any controls,
      // or cover any other windows.
      const ImGuiWindowFlags windowFlags =
          ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
          ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

      ImGuiViewport *viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(viewport->Pos);
      ImGui::SetNextWindowSize(viewport->Size);
      ImGui::SetNextWindowViewport(viewport->ID);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

      static const char *dockSpaceName = "VKLExamplesDocker";
      const ImGuiID dockSpaceId        = ImGui::GetID(dockSpaceName);

      ImGui::Begin(dockSpaceName, nullptr, windowFlags);

      // Initially, we use only two nodes: left for scene controls, center
      // for views.
      if (ImGui::DockBuilderGetNode(dockSpaceId) == nullptr) {
        ImGui::DockBuilderRemoveNode(dockSpaceId);
        ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockSpaceId,
                                      ImGui::GetMainViewport()->Size);

        ImGuiID ctr = 0;
        ImGui::DockBuilderSplitNode(
            dockSpaceId, ImGuiDir_Left, 0.28f, &leftNodeId, &ctr);

        ImGui::DockBuilderSplitNode(
            ctr, ImGuiDir_Right, 0.28f, &rightNodeId, &centerNodeId);

        ImGui::DockBuilderFinish(dockSpaceId);
      }

      // Create the actual dock space.
      ImGui::DockSpace(dockSpaceId, ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None);

      ImGui::End();
      ImGui::PopStyleVar(3);
    }

    void InteractiveApplication::disableInvisibleRenderViews()
    {
      for (auto it = activeViews.begin(); it != activeViews.end();) {
        auto *view = *it;
        if (view->getVisibleFlag()) {
          ++it;
        } else {
          scheduler->stop(view->getRenderer());
          inactiveViews.push_back(view);
          it = activeViews.erase(it);
        }
      }
    }

    void InteractiveApplication::enableVisibleRenderViews()
    {
      for (auto it = inactiveViews.begin(); it != inactiveViews.end();) {
        auto *view = *it;
        if (view->getVisibleFlag()) {
          scheduler->start(view->getRenderer());
          activeViews.push_back(view);
          it = inactiveViews.erase(it);
        } else {
          ++it;
        }
      }
    }

  }  // namespace examples
}  // namespace openvkl

