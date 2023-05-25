// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "RayMarchIteratorGpu.h"

namespace openvkl {
  namespace examples {

    // required for specialization constants
    static_assert(std::is_trivially_copyable<VKLFeatureFlags>::value);

    constexpr static sycl::specialization_id<VKLFeatureFlags> samplerSpecId{
        VKL_FEATURE_FLAGS_DEFAULT};

    RayMarchIteratorGpu::RayMarchIteratorGpu(Scene &scene) : RendererGpu{scene}
    {
      intervalContext =
          vklNewIntervalIteratorContext(scene.volume.getSampler());
      updateIntervalContext();
    }

    RayMarchIteratorGpu::~RayMarchIteratorGpu()
    {
      scheduler.stop(*this);
      deallocateBuffers();
      vklRelease(intervalContext);
    }

    void RayMarchIteratorGpu::beforeFrame(bool &needToClear)
    {
      RendererGpu::beforeFrame(needToClear);

      scheduler.locked(guiParams, [&]() {
        needToClear |= params.updateIfChanged(guiParams);
      });

      // Renderer params contain the attribute index. This is why
      // we update the interval context when the parent changed.
      if (needToClear) {
        updateIntervalContext();
      }
    }

    void RayMarchIteratorGpu::updateIntervalContext()
    {
      vklSetInt(
          intervalContext, "attributeIndex", rendererParams->attributeIndex);

      vklSetFloat(intervalContext,
                  "intervalResolutionHint",
                  params->intervalResolutionHint);

      // set interval context value ranges based on transfer function positive
      // opacity intervals, if we have any
      VKLData valueRangesData;

      const std::vector<range1f> valueRanges =
          rendererParams->transferFunction.getPositiveOpacityValueRanges();

      if (!valueRanges.empty()) {
        valueRangesData = vklNewData(getOpenVKLDevice(),
                                     valueRanges.size(),
                                     VKL_BOX1F,
                                     valueRanges.data());
      }

      vklSetData(intervalContext, "valueRanges", valueRangesData);

      if (valueRangesData) {
        vklRelease(valueRangesData);
      }

      vklCommit(intervalContext);

      iteratorSize = vklGetIntervalIteratorSize(&intervalContext);
    }

    void RayMarchIteratorGpu::reallocateBuffers(const size_t width,
                                                const size_t height)
    {
      deallocateBuffers();
      // allocate buffer for interval iterator for each pixel
      iteratorBuffer = sycl::malloc_device<char>(width * height * iteratorSize,
                                                 getSyclQueue());
    }

    void RayMarchIteratorGpu::deallocateBuffers()
    {
      sycl::free(iteratorBuffer, getSyclQueue());
      iteratorBuffer = nullptr;
    }

    void RayMarchIteratorGpu::onBufferResize(const size_t width,
                                             const size_t height)
    {
      RendererGpu::onBufferResize(width, height);
      reallocateBuffers(width, height);
    }

    void RayMarchIteratorGpu::setKernelObjectAttributes(
        RayMarchIteratorGpuKernel *gpuKernelRenderer)
    {
      gpuKernelRenderer->setObjectAttributes(scene.volume.getSampler(),
                                             scene.volume.getBounds(),
                                             *rendererParams,
                                             *params);
    }

    sycl::event RayMarchIteratorGpu::invokeGpuRenderPixel(
        sycl::queue &syclQueue,
        BufferGpu &buffer,
        const Ray *rayBuffer,
        const bool clearFramebuffer)
    {
      const unsigned int width  = buffer.getWidth();
      const unsigned int height = buffer.getHeight();
      auto rgbaBuffer           = buffer.getRgba();
      auto weightBuffer         = buffer.getWeight();

      RayMarchIteratorGpuKernel *gpuKernelRendererDevice =
          prepareGpuKernelObject();

      assert(iteratorBuffer);
      assert(intervalContext);
      char *_iteratorBuffer                             = iteratorBuffer;
      const VKLIntervalIteratorContext _intervalContext = intervalContext;
      const size_t _iteratorSize                        = iteratorSize;

      auto gpuKernelFunction = [=](unsigned int idx, sycl::kernel_handler kh) {
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
            idx,
            ray,
            *rgba,
            *weight,
            _iteratorBuffer + idx * _iteratorSize,
            _intervalContext,
            featureFlags);
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
