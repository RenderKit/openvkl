// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DensityPathTracerGpu.h"

namespace openvkl {
  namespace examples {

    DensityPathTracerGpu::DensityPathTracerGpu(Scene &scene)
        : RendererGpu{scene}
    {
      gpuKernelRendererDevice =
          sycl::malloc_device<DensityPathTracerGpuKernel>(1, getSyclQueue());

      gpuKernelRendererHost =
          sycl::malloc_host<DensityPathTracerGpuKernel>(1, getSyclQueue());
    }

    DensityPathTracerGpu::~DensityPathTracerGpu()
    {
      scheduler.stop(*this);

      if (gpuKernelRendererDevice) {
        sycl::free(gpuKernelRendererDevice, getSyclQueue());
        gpuKernelRendererDevice = nullptr;
      }
      if (gpuKernelRendererHost) {
        sycl::free(gpuKernelRendererHost, getSyclQueue());
        gpuKernelRendererHost = nullptr;
      }
    }

    void DensityPathTracerGpu::beforeFrame(bool &needToClear)
    {
      RendererGpu::beforeFrame(needToClear);

      bool paramsChanged = false;
      scheduler.locked(guiParams, [&]() {
        paramsChanged = params.updateIfChanged(guiParams);
      });

      needToClear |= paramsChanged;
    }

    void DensityPathTracerGpu::afterFrame()
    {
      ++frameId;
    }

    sycl::event DensityPathTracerGpu::invokeGpuRenderPixel(
        sycl::queue &syclQueue,
        BufferGpu &buffer,
        const Ray *rayBuffer,
        const bool clearFramebuffer)
    {
      const unsigned int width  = buffer.getWidth();
      const unsigned int height = buffer.getHeight();
      auto rgbaBuffer           = buffer.getRgba();
      auto weightBuffer         = buffer.getWeight();

      // Update all attributes - not all of them needs that.
      // 'frameId' is updated on each frame so we need to send this update
      // anyway.
      gpuKernelRendererHost->setObjectAttributes(
          *(scene.volume.getSamplerPtr()),
          scene.volume.getBounds(),
          *params,
          *rendererParams,
          frameId);

      syclQueue.memcpy(gpuKernelRendererDevice,
                       gpuKernelRendererHost,
                       sizeof(DensityPathTracerGpuKernel));

      DensityPathTracerGpuKernel *_gpuKernelRendererDevice =
          gpuKernelRendererDevice;

      auto gpuKernelFunction = [=](auto idx) {
        const int y = idx / width;
        const int x = idx % width;

        if (!_gpuKernelRendererDevice->pixelOnScene(idx, width, height, x, y)) {
          rgbaBuffer[idx]   = vec4f(.18f, .18f, .18f, 1.f);
          weightBuffer[idx] = 1.f;
          return;
        }

        if (clearFramebuffer) {
          rgbaBuffer[idx]   = vec4f(0.f);
          weightBuffer[idx] = 0.f;
        }

        const Ray *ray = &rayBuffer[idx];
        vec4f *rgba    = &rgbaBuffer[idx];
        float *weight  = &weightBuffer[idx];

        _gpuKernelRendererDevice->renderPixel(idx, ray, *rgba, *weight);
      };

      return syclQueue.parallel_for(width * height, gpuKernelFunction);
    }

  }  // namespace examples
}  // namespace openvkl
