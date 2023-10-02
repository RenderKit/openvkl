// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "RenderView.h"
#include "renderer/Renderer.h"

namespace openvkl {
  namespace examples {

    RenderView::RenderView(const std::string &name,
                           std::unique_ptr<Renderer> &&_renderer)
        : name{name},
          toolboxName{name + "_TOOLBOX"},
          renderer{std::forward<std::unique_ptr<Renderer>>(_renderer)}
    {
      parameterGui = ParameterGui::makeRendererGui(renderer.get());

      glGenTextures(1, &fbTexId);
      glBindTexture(GL_TEXTURE_2D, fbTexId);
      // NEAREST is important, we prefer seeing what is actually in the
      // framebuffer.
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    RenderView::~RenderView()
    {
      glDeleteTextures(1, &fbTexId);
    }

    bool getDragLine(
        ImGuiIO &io, const vec2i &origin, int button, vec2f &from, vec2f &to)
    {
      const ImVec2 delta = ImGui::GetMouseDragDelta(button);
      bool wasDragged    = (delta.x != 0 && delta.y != 0);
      if (wasDragged) {
        const ImVec2 start = io.MouseClickedPos[button];

        from = vec2f(start.x - origin.x, start.y - origin.y);
        to = vec2f(start.x - origin.x + delta.x, start.y - origin.y + delta.y);
      }
      return wasDragged;
    }

    bool RenderView::draw()
    {
      if (visible) {
        ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        // I am unclear why this is required, but drawing vertically centered
        // framebuffers that are too wide for the canvas somehow adds empty
        // space at the bottom of the window.
        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        auto &scene     = getRenderer().getScene();
        auto &scheduler = scene.scheduler;

        scheduler.renderFrame(getRenderer());

        if (ImGui::Begin(name.c_str(), &visible, flags)) {
          auto &camera = scene.camera;

          const ImVec2 origin = ImGui::GetCursorScreenPos();
          const ImVec2 size   = ImGui::GetContentRegionAvail();

          // This button is here to catch mouse events, instead of just checking
          // IsWindowFocused (which would rotate the camera when moving a
          // floating window).
          ImGui::InvisibleButton("canvas",
                                 size,
                                 ImGuiButtonFlags_MouseButtonLeft |
                                     ImGuiButtonFlags_MouseButtonMiddle |
                                     ImGuiButtonFlags_MouseButtonRight);

          const bool isHovered = ImGui::IsItemHovered();
          const bool isHeld    = ImGui::IsItemActive();
          const vec2i vorg(origin.x, origin.y);
          const vec2i vsize(size.x, size.y);

          ImGuiIO &io = ImGui::GetIO();
          vec2f from, to;
          if (isHeld &&
              getDragLine(io, vorg, ImGuiMouseButton_Left, from, to)) {
            scheduler.locked(camera, [&]() {
              camera->rotate(from, to, vsize);
              camera.incrementVersion();
            });
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
          }
          if (isHeld &&
              getDragLine(io, vorg, ImGuiMouseButton_Middle, from, to)) {
            scheduler.locked(camera, [&]() {
              camera->pan(from, to, vsize);
              camera.incrementVersion();
            });
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
          }
          if (isHeld &&
              getDragLine(io, vorg, ImGuiMouseButton_Right, from, to)) {
            scheduler.locked(camera, [&]() {
              camera->dolly(from, to, vsize);
              camera.incrementVersion();
            });
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
          }

          drawBackground(origin, size);
          drawFramebuffer(scheduler, origin, size);
        }
        ImGui::End();
        ImGui::PopStyleVar(1);
      }
      return visible;
    }

    void RenderView::drawParameterGui()
    {
      if (renderer && parameterGui) {
        auto &scene     = renderer->getScene();
        auto &scheduler = scene.scheduler;
        parameterGui->draw(scheduler);
      }
    }

    void RenderView::drawBackground(const ImVec2 &canvasOrigin,
                                    const ImVec2 &canvasSize) const
    {
      const ImVec2 p0 = canvasOrigin;
      const ImVec2 p1(canvasOrigin.x + canvasSize.x,
                      canvasOrigin.y + canvasSize.y);

      ImGuiIO &io          = ImGui::GetIO();
      ImDrawList *drawList = ImGui::GetWindowDrawList();
      drawList->AddRectFilled(p0, p1, IM_COL32(30, 30, 30, 255));
    }

    void RenderView::drawFramebuffer(const Scheduler &scheduler,
                                     const ImVec2 &canvasOrigin,
                                     const ImVec2 &canvasSize) const
    {
      if (canvasSize.x < 1 || canvasSize.y < 1) {
        return;
      }

      if (renderer) {
        // Request a framebuffer that matches available space!
        const auto &framebuffer =
            renderer->getFramebuffer(canvasSize.x, canvasSize.y);
        const auto &frontBuffer = framebuffer.getFrontBuffer();

        size_t fbw = 0;
        size_t fbh = 0;
        Stats stats;

        // Upload to GPU.
        glBindTexture(GL_TEXTURE_2D, fbTexId);
        scheduler.locked(frontBuffer, [&]() {
          glTexImage2D(GL_TEXTURE_2D,
                       0,
                       GL_RGBA,
                       frontBuffer.getWidth(),
                       frontBuffer.getHeight(),
                       0,
                       GL_RGBA,
                       GL_FLOAT,
                       frontBuffer.getRgba());
          fbw   = frontBuffer.getWidth();
          fbh   = frontBuffer.getHeight();
          stats = frontBuffer.getStats();
        });
        glBindTexture(GL_TEXTURE_2D, 0);

        // We have no guarantee that the renderer actually respected our
        // width / height, so fit the resulting texture into the available
        // space.
        const float fbAspectRatio = fbw / static_cast<float>(fbh);
        const float caAspectRatio =
            canvasSize.x / static_cast<float>(canvasSize.y);

        ImVec2 imgOrigin, imgSize;
        if (caAspectRatio > fbAspectRatio) {
          // Framebuffer is narrower than canvas. Match height,
          // center horizontally.
          const float ww =
              std::min<float>(canvasSize.y * fbAspectRatio, canvasSize.x);
          imgOrigin.x = canvasOrigin.x + 0.5f * (canvasSize.x - ww);
          imgOrigin.y = canvasOrigin.y;
          imgSize.x   = ww;
          imgSize.y   = canvasSize.y;
        } else {
          // Framebuffer is wider than canvas. Match width, center vertically.
          const float hh =
              std::min<float>(canvasSize.x / fbAspectRatio, canvasSize.y);
          imgOrigin.x = canvasOrigin.x;
          imgOrigin.y = canvasOrigin.y + 0.5f * (canvasSize.y - hh);
          imgSize.x   = canvasSize.x;
          imgSize.y   = hh;
        }

        ImGui::SetCursorScreenPos(ImVec2(imgOrigin.x, imgOrigin.y));
        ImGui::Image((void *)(intptr_t)fbTexId,
                     imgSize,
                     // Mirror the image. This way, we can just render images
                     // naturally and they will the right side up both in the
                     // viewport and when exporting the framebuffer to disk.
                     ImVec2(1, 1),
                     ImVec2(0, 0));

        drawStats(stats, vec2i(imgOrigin.x + 10, imgOrigin.y + 10));
        if (renderer->getScene().printStats) {
          stats.printToStdout();
        }
      }
    }

    // -------------------------------------------------------------------------

    inline double toSeconds(const Stats::Clock::duration &d)
    {
      using MuS = std::chrono::microseconds;
      return std::chrono::duration_cast<MuS>(d).count() / 1000000.0;
    }

    inline double toMilliseconds(const Stats::Clock::duration &d)
    {
      using MuS = std::chrono::microseconds;
      return std::chrono::duration_cast<MuS>(d).count() / 1000.0;
    }

    void RenderView::drawStats(const Stats &stats, const vec2i &origin) const
    {
      ImDrawList *drawList = ImGui::GetWindowDrawList();
      std::ostringstream os;
      os << std::showpoint << std::fixed << std::setprecision(2);
      if (stats.frameTime.count() > 0) {
        const int fps = static_cast<int>(1.0 / toSeconds(stats.frameTime));
        os << "fps (frame)      : " << fps << "\n";
      }
      if (stats.renderTime.count() > 0) {
        const int fps = static_cast<int>(1.0 / toSeconds(stats.renderTime));
        os << "fps (rendering)  : " << fps << "\n";
        os << "rendering        : " << std::setw(5)
           << toMilliseconds(stats.renderTime) << " ms\n";
      }
      if (stats.copyTime.count() > 0) {
        os << "framebuffer copy : " << std::setw(5)
           << toMilliseconds(stats.copyTime) << " ms\n";
      }
      if (stats.tonemapTime.count() > 0) {
        os << "tonemapping      : " << std::setw(5)
           << toMilliseconds(stats.tonemapTime) << " ms\n";
      }
      const ImVec2 textOrigin(origin.x, origin.y);
      drawList->AddText(
          textOrigin, IM_COL32(255, 255, 255, 100), os.str().c_str());
    }

  }  // namespace examples
}  // namespace openvkl
