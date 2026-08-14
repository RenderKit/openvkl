// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Dockspace.h"

#include <imgui.h>
#include <imgui_internal.h>  // For dockspace API.

namespace openvkl {
  namespace examples {

    void initDockspace(unsigned &leftNodeId,
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

  }  // namespace examples
}  // namespace openvkl
