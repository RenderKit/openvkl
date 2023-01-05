// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "RendererGpu.h"

namespace openvkl {
  namespace examples {
    RendererGpu::RendererGpu(Scene &scene) : Renderer{scene}
    {
      syclQueue = getSyclQueue();
    }

    RendererGpu::~RendererGpu()
    {
      deallocateBuffers();
    }

    void RendererGpu::reallocateBuffers(size_t width, size_t height)
    {
      deallocateBuffers();
      rayBuffer     = sycl::malloc_device<Ray>(width * height, syclQueue);
      rayBufferHost = sycl::malloc_host<Ray>(width * height, syclQueue);
    }

    void RendererGpu::deallocateBuffers()
    {
      sycl::free(rayBuffer, syclQueue);
      rayBuffer = nullptr;

      sycl::free(rayBufferHost, syclQueue);
      rayBufferHost = nullptr;
    }

    const Framebuffer &RendererGpu::getFramebuffer(size_t width, size_t height)
    {
      // Note: This is all in the main thread, so no need to lock any parameters
      // here.
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
        reallocateBuffers(width, height);
        scheduler.start(*this);
        // Trigger render frame after resizing buffer
        // with clearFramebuffer flag set to true.
        renderFrameImpl(true);
      }

      return framebuffer;
    }

    sycl::event RendererGpu::invokeGpuRayGeneration(const unsigned int width,
                                                    const unsigned int height)
    {
      rkcommon::tasking::parallel_in_blocks_of<16>(
          width * height, [&](size_t ib, size_t ie) {
            for (size_t idx = ib; idx < ie; ++idx) {
              const int y = idx / width;
              const int x = idx % width;

              Ray *ray = &rayBufferHost[idx];
              cam->createRay(
                  vec2f(x, y), vec2i(width, height), ray->org, ray->dir);
            }
          });

      return syclQueue.memcpy(
          rayBuffer, rayBufferHost, sizeof(Ray) * width * height);
    }

    void RendererGpu::renderFrameImpl(bool clearFramebuffer)
    {
      const auto startFrame = Stats::Clock::now();
      if (scheduler.workerMustTerminate(*this)) {
        return;
      }

      const size_t width  = framebuffer.getWidth();
      const size_t height = framebuffer.getHeight();

      if (width == 0 || height == 0) {
        return;
      }

      if (clearFramebuffer) {
        invokeGpuRayGeneration(width, height);
      }

      auto &backBuffer = framebuffer.getBackBuffer();

      const auto startRenderPixel = Stats::Clock::now();

      const auto gpuRenderPixelEvent = invokeGpuRenderPixel(
          syclQueue, backBuffer, rayBuffer, clearFramebuffer);

      syclQueue.wait();

      const auto endRenderPixel = Stats::Clock::now();

      auto &fBuf = framebuffer.getFrontBuffer();
      scheduler.locked(fBuf, [&]() {
        backBuffer.tonemap(fBuf);

        const auto endFrame = Stats::Clock::now();

        fBuf.getStats().frameTime = endFrame - startFrame;

        // To get only GPU exec time -> Stats::convert(gpuRenderPixelEvent);
        fBuf.getStats().renderTime = endRenderPixel - startRenderPixel;
      });
    }
  }  // namespace examples
}  // namespace openvkl