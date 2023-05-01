// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DensityPathTracerGpu.h"

namespace openvkl {
  namespace examples {

    // required for specialization constants
    static_assert(std::is_trivially_copyable<VKLFeatureFlags>::value);

    constexpr static sycl::specialization_id<VKLFeatureFlags> samplerSpecId{
        VKL_FEATURE_FLAG_ALL};

    DensityPathTracerGpu::DensityPathTracerGpu(Scene &scene)
        : RendererGpu{scene}
    {
    }

    DensityPathTracerGpu::~DensityPathTracerGpu()
    {
      scheduler.stop(*this);
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

    void DensityPathTracerGpu::setKernelObjectAttributes(
        DensityPathTracerGpuKernel *gpuKernelRenderer)
    {
      // Update all attributes - not all of them needs that.
      // 'frameId' is updated on each frame so we need to send this update
      // anyway.
      gpuKernelRenderer->setObjectAttributes(scene.volume.getSampler(),
                                             scene.volume.getBounds(),
                                             *rendererParams,
                                             *params,
                                             frameId);
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

      DensityPathTracerGpuKernel *gpuKernelRendererDevice =
          prepareGpuKernelObject();

      auto gpuKernelFunction = [=](auto idx, sycl::kernel_handler kh) {
        bool skipPixelRendering = gpuKernelRendererDevice->bufferPreprocessing(
            clearFramebuffer, idx, width, height, rgbaBuffer, weightBuffer);

        if (skipPixelRendering) {
          return;
        }

        const Ray *ray = &rayBuffer[idx];
        vec4f *rgba    = &rgbaBuffer[idx];
        float *weight  = &weightBuffer[idx];

        const VKLFeatureFlags featureFlags =
            kh.get_specialization_constant<samplerSpecId>();

        gpuKernelRendererDevice->renderPixel(
            idx, ray, *rgba, *weight, featureFlags);
      };

      VKLFeatureFlags requiredFeatures =
          vklGetFeatureFlags(getScene().volume.getSampler());

      return syclQueue.submit([=](sycl::handler &cgh) {
        cgh.set_specialization_constant<samplerSpecId>(requiredFeatures);

        cgh.parallel_for(width * height, gpuKernelFunction);
      });
    }
  }  // namespace examples
}  // namespace openvkl
