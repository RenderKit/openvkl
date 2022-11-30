// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.h"

#include "Renderer_ispc.h"
#include "Scene_ispc.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace openvkl {
  namespace examples {

    Renderer::Renderer(Scene &scene) : scene(scene), scheduler(scene.scheduler)
    {
      // Note:
      // Do not start the renderer here - the render functions don't exist
      // until the derived classes have been constructed.
      assert(!run.load());
    }

    Renderer::~Renderer()
    {
      // Note:
      // Do not stop the renderer here - the render functions don't exist
      // any more.
      assert(!run.load());
    }

    void Renderer::beforeFrame(bool &needToClear)
    {
      bool cameraChanged = false;
      scheduler.locked(scene.camera, [&]() {
        cameraChanged = cam.updateIfChanged(scene.camera);
      });

      bool rendererParamsChanged = false;
      scheduler.locked(scene.rendererParams, [&]() {
        rendererParamsChanged =
            rendererParams.updateIfChanged(scene.rendererParams);
      });

      needToClear |= cameraChanged || rendererParamsChanged;
    }

    vec4f Renderer::sampleTransferFunction(float value) const
    {
      vec4f colorAndOpacity{0.f};

      const auto &valueRange = rendererParams->transferFunction.valueRange;
      const auto &colorsAndOpacities =
          rendererParams->transferFunction.colorsAndOpacities;

      if (std::isnan(value) || colorsAndOpacities.empty()) {
        return colorAndOpacity;
      }

      if (value <= valueRange.lower) {
        return colorsAndOpacities[0];
      }

      if (value >= valueRange.upper) {
        return colorsAndOpacities.back();
      }

      // map the value into the range [0, size - 1]
      value = (value - valueRange.lower) /
              (valueRange.upper - valueRange.lower) *
              (colorsAndOpacities.size() - 1);

      // index and fractional offset
      const int index       = floor(value);
      const float remainder = value - index;

      // the final interpolated value
      return ((1.f - remainder) * colorsAndOpacities[index] +
              remainder * colorsAndOpacities[min(
                              index + 1, int(colorsAndOpacities.size() - 1))]);
    }

    // -------------------------------------------------------------------------

    HostRenderer::HostRenderer(Scene &scene) : Renderer{scene}
    {
    }

    HostRenderer::~HostRenderer()
    {
    }

    const Framebuffer &HostRenderer::getFramebuffer(size_t w, size_t h)
    {
      // Note: This is all in the main thread, so no need to lock any parameters
      // here.
      size_t width  = w;
      size_t height = h;

      const auto &params = *(scene.rendererParams);
      if (params.fixedFramebufferSize) {
        width  = params.framebufferSize.x;
        height = params.framebufferSize.y;
      }

      const bool resolutionChanged = (framebuffer.getWidth() != width ||
                                      framebuffer.getHeight() != height);

      if (resolutionChanged) {
        scheduler.stop(*this);
        framebuffer.resize(width, height);
        scheduler.start(*this);
      }

      return framebuffer;
    }

    // -------------------------------------------------------------------------

    ScalarRenderer::ScalarRenderer(Scene &scene) : HostRenderer{scene} {}

    void ScalarRenderer::renderFrameImpl(bool clearFramebuffer)
    {
      const auto startFrame = Framebuffer::Stats::Clock::now();

      if (scheduler.workerMustTerminate(*this)) {
        return;
      }

      auto &bBuf = framebuffer.getBackBuffer();
      auto &fBuf = framebuffer.getFrontBuffer();

      scheduler.locked(bBuf, [&]() {
        const size_t ww = bBuf.getWidth();
        const size_t hh = bBuf.getHeight();

        if (ww == 0 || hh == 0) {
          return;
        }

        const auto startRender = Framebuffer::Stats::Clock::now();
        rkcommon::tasking::parallel_in_blocks_of<16>(
            ww * hh, [&](size_t ib, size_t ie) {
              if (scheduler.workerMustTerminate(*this)) {
                return;
              }

              for (size_t idx = ib; idx < ie; ++idx) {
                const int y   = idx / ww;
                const int x   = idx % ww;
                vec4f &rgba   = bBuf.getRgba()[idx];
                float &weight = bBuf.getWeight()[idx];

                if (rendererParams->fixedFramebufferSize &&
                    rendererParams->restrictPixelRange) {
                  // The output is mirrored!
                  if ((hh - y - 1) < rendererParams->pixelRange.lower.y ||
                      (hh - y - 1) >= rendererParams->pixelRange.upper.y ||
                      (ww - x - 1) < rendererParams->pixelRange.lower.x ||
                      (ww - x - 1) >= rendererParams->pixelRange.upper.x) {
                    rgba   = vec4f(.18f, .18f, .18f, 1.f);
                    weight = 1.f;
                    continue;
                  }
                }

                if (clearFramebuffer) {
                  rgba   = vec4f(0.f);
                  weight = 0.f;
                }

                Ray ray;
                // NOTE: Should use filtered IS at some point.
                cam->createRay(vec2f(x, y), vec2i(ww, hh), ray.org, ray.dir);
                const size_t seed = ww * static_cast<size_t>(y) + x;
                renderPixel(seed, ray, rgba, weight);
              }
            });
        const auto endRender = Framebuffer::Stats::Clock::now();

        // Copy output into the front buffer and prepare for display!
        scheduler.locked(fBuf, [&]() {
          fBuf.copy(bBuf);
          fBuf.tonemapInPlace();
          fBuf.getStats().renderTime = endRender - startRender;
          const auto endFrame        = Framebuffer::Stats::Clock::now();
          fBuf.getStats().frameTime  = endFrame - startFrame;
        });
      });
    }

    // -------------------------------------------------------------------------

    IspcRenderer::IspcRenderer(Scene &scene) : HostRenderer{scene}
    {
      ispcScene = ispc::Scene_create();
    }

    IspcRenderer::~IspcRenderer()
    {
      ispc::Scene_destroy(ispcScene);
      ispcScene = nullptr;
    }

    void IspcRenderer::beforeStart()
    {
      HostRenderer::beforeStart();

      ispc::Scene_setVolume(ispcScene, scene.volume.getVolumePtr());
      ispc::Scene_setSampler(ispcScene, scene.volume.getSamplerPtr());
    }

    void IspcRenderer::beforeFrame(bool &needToClear)
    {
      bool parentNeedsToClear = false;
      HostRenderer::beforeFrame(parentNeedsToClear);

      if (parentNeedsToClear) {
        ispc::Scene_setRendererParams(
            ispcScene,
            rendererParams->attributeIndex,
            rendererParams->time,
            rendererParams->fixedFramebufferSize &&
                rendererParams->restrictPixelRange,
            reinterpret_cast<const ispc::box2i &>(rendererParams->pixelRange));

        ispc::Scene_setTransferFunction(
            ispcScene,
            reinterpret_cast<const ispc::box1f &>(
                rendererParams->transferFunction.valueRange),
            rendererParams->transferFunction.colorsAndOpacities.size(),
            reinterpret_cast<const ispc::vec4f *>(
                rendererParams->transferFunction.colorsAndOpacities.data()));

        const AffineSpace3f ctw = cam->getCameraToWorld();
        const vec3f ctw_R0      = ctw.l.row0();
        const vec3f ctw_R1      = ctw.l.row1();
        const vec3f ctw_R2      = ctw.l.row2();

        ispc::Scene_setCamera(ispcScene,
                              cam->getSensorWidth(),
                              cam->getFocalLength(),
                              reinterpret_cast<const ispc::vec3f &>(ctw_R0),
                              reinterpret_cast<const ispc::vec3f &>(ctw_R1),
                              reinterpret_cast<const ispc::vec3f &>(ctw_R2),
                              reinterpret_cast<const ispc::vec3f &>(ctw.p));

        needToClear = true;
      }
    }

    void IspcRenderer::renderFrameImpl(bool clearFramebuffer)
    {
      const auto startFrame = Framebuffer::Stats::Clock::now();

      if (scheduler.workerMustTerminate(*this)) {
        return;
      }

      auto &bBuf    = framebuffer.getBackBuffer();
      vec4f *rgba   = bBuf.getRgba();
      float *weight = bBuf.getWeight();

      auto &fBuf = framebuffer.getFrontBuffer();

      scheduler.locked(bBuf, [&]() {
        const size_t ww = bBuf.getWidth();
        const size_t hh = bBuf.getHeight();

        if (ww == 0 || hh == 0) {
          return;
        }

        const size_t numPixels    = ww * hh;
        const size_t pixelsPerJob = ispc::Renderer_pixelsPerJob();
        const size_t numJobs      = numPixels / pixelsPerJob;

        const auto startRender = Framebuffer::Stats::Clock::now();
        rkcommon::tasking::parallel_for(numJobs, [&](size_t i) {
          if (scheduler.workerMustTerminate(*this)) {
            return;
          }

          const size_t ib = i * pixelsPerJob;

          if (clearFramebuffer) {
            std::memset(rgba + ib, 0, sizeof(vec4f) * pixelsPerJob);
            std::memset(weight + ib, 0, sizeof(float) * pixelsPerJob);
          }

          renderPixelBlock(
              vec2i(ww, hh), static_cast<uint32_t>(ib), rgba, weight);
        });

        const auto endRender = Framebuffer::Stats::Clock::now();

        // Copy output into the front buffer and prepare for display!
        scheduler.locked(fBuf, [&]() {
          fBuf.copy(bBuf);
          fBuf.tonemapInPlace();
          fBuf.getStats().renderTime = endRender - startRender;
          const auto endFrame        = Framebuffer::Stats::Clock::now();
          fBuf.getStats().frameTime  = endFrame - startFrame;
        });
      });
    }

  }  // namespace examples
}  // namespace openvkl
