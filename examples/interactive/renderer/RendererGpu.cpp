// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "RendererGpu.h"
#include "density_path_tracer/DensityPathTracerGpuKernel.h"
#include "hit_iterator_renderer/HitIteratorRendererGpuKernel.h"
#include "interval_iterator_debug/IntervalIteratorDebugGpuKernel.h"
#include "ray_march_iterator/RayMarchIteratorGpuKernel.h"

namespace openvkl {
  namespace examples {
    template <typename TRendererGpuKernelSubtype, typename TRenderParamsType>
    RendererGpu<TRendererGpuKernelSubtype, TRenderParamsType>::RendererGpu(Scene &scene)
        : Renderer{scene}
    {
      syclQueue = getSyclQueue();
      gpuKernelRendererDevice =
          sycl::malloc_device<TRendererGpuKernelSubtype>(1, syclQueue);
      gpuKernelRendererHost =
          sycl::malloc_host<TRendererGpuKernelSubtype>(1, syclQueue);
    }

    template <typename TRendererGpuKernelSubtype, typename TRenderParamsType>
    RendererGpu<TRendererGpuKernelSubtype, TRenderParamsType>::~RendererGpu()
    {
      deallocateBuffers();

      sycl::free(gpuKernelRendererDevice, syclQueue);
      gpuKernelRendererDevice = nullptr;

      sycl::free(gpuKernelRendererHost, syclQueue);
      gpuKernelRendererHost = nullptr;
    }

    template <typename TRendererGpuKernelSubtype, typename TRenderParamsType>
    void RendererGpu<TRendererGpuKernelSubtype, TRenderParamsType>::reallocateBuffers(
        size_t width, size_t height)
    {
      deallocateBuffers();
      rayBuffer     = sycl::malloc_device<Ray>(width * height, syclQueue);
      rayBufferHost = sycl::malloc_host<Ray>(width * height, syclQueue);
    }

    template <typename TRendererGpuKernelSubtype, typename TRenderParamsType>
    void RendererGpu<TRendererGpuKernelSubtype, TRenderParamsType>::deallocateBuffers()
    {
      sycl::free(rayBuffer, syclQueue);
      rayBuffer = nullptr;

      sycl::free(rayBufferHost, syclQueue);
      rayBufferHost = nullptr;
    }

    template <typename TRendererGpuKernelSubtype, typename TRenderParamsType>
    void RendererGpu<TRendererGpuKernelSubtype, TRenderParamsType>::onBufferResize(
        const size_t width, const size_t height)
    {
      framebuffer.resize(width, height);
      reallocateBuffers(width, height);

      // Set clear framebuffer flag so buffer
      // will be cleared in next renderFrame.
      setClearFramebufferFlag();
    }

    template <typename TRendererGpuKernelSubtype, typename TRenderParamsType>
    const Framebuffer &RendererGpu<TRendererGpuKernelSubtype, TRenderParamsType>::getFramebuffer(
        size_t width, size_t height)
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
        onBufferResize(width, height);
        scheduler.start(*this);
      }

      return framebuffer;
    }

    template <typename TRendererGpuKernelSubtype, typename TRenderParamsType>
    sycl::event RendererGpu<TRendererGpuKernelSubtype, TRenderParamsType>::invokeGpuRayGeneration(
        const unsigned int width, const unsigned int height)
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

    template <typename TRendererGpuKernelSubtype, typename TRenderParamsType>
    void RendererGpu<TRendererGpuKernelSubtype, TRenderParamsType>::afterFrame()
    {
      ++frameId;
    }

    template <typename TRendererGpuKernelSubtype, typename TRenderParamsType>
    void RendererGpu<TRendererGpuKernelSubtype, TRenderParamsType>::renderFrameImpl(
        bool clearFramebuffer)
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

    template class RendererGpu<RayMarchIteratorGpuKernel, RayMarchIteratorParams>;
    template class RendererGpu<DensityPathTracerGpuKernel, DensityPathTracerParams>;
    template class RendererGpu<HitIteratorRendererGpuKernel, HitIteratorRendererParams>;
    template class RendererGpu<IntervalIteratorDebugGpuKernel, IntervalIteratorDebugParams>;

  }  // namespace examples
}  // namespace openvkl