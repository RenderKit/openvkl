// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <list>
#include <vector>

struct GLFWwindow;

namespace openvkl {
  namespace examples {

    struct Scene;
    class Scheduler;
    class RenderView;

    class InteractiveApplication
    {
     public:
      InteractiveApplication();
      ~InteractiveApplication();
      void run(Scene &scene);

     private:
      void initializeImgui();
      void finalizeImgui();
      bool createWindow(bool disableVSync);
      void initDockspace(unsigned &leftNodeId,
                         unsigned &centerNodeId,
                         unsigned &rightNodeId);
      void showContextMenu();
      void disableInvisibleRenderViews();
      void enableVisibleRenderViews();

     private:
      GLFWwindow *window{nullptr};
      Scheduler *scheduler{nullptr};
      std::list<RenderView *> activeViews;
      std::list<RenderView *> inactiveViews;
      std::vector<std::unique_ptr<RenderView>> views;
    };

  }  // namespace examples
}  // namespace openvkl

