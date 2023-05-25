// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "HitIteratorRendererGpu.h"

namespace openvkl {
  namespace examples {

    // required for specialization constants
    static_assert(std::is_trivially_copyable<VKLFeatureFlags>::value);

    constexpr static sycl::specialization_id<VKLFeatureFlags> samplerSpecId{
        VKL_FEATURE_FLAGS_DEFAULT};

    HitIteratorRendererGpu::HitIteratorRendererGpu(Scene &scene)
        : RendererGpu{scene}
    {
      hitContext = vklNewHitIteratorContext(scene.volume.getSampler());
      updateHitContext();
    }

    HitIteratorRendererGpu::~HitIteratorRendererGpu()
    {
      scheduler.stop(*this);
      deallocateBuffers();
      vklRelease(hitContext);
    }

    void HitIteratorRendererGpu::beforeFrame(bool &needToClear)
    {
      RendererGpu::beforeFrame(needToClear);

      scheduler.locked(guiParams, [&]() {
        needToClear |= params.updateIfChanged(guiParams);
      });

      // Renderer params contain the attribute index. This is why
      // we update the interval context when the parent changed.
      if (needToClear) {
        updateHitContext();
      }
    }

    void HitIteratorRendererGpu::updateHitContext()
    {
      vklSetInt(hitContext, "attributeIndex", rendererParams->attributeIndex);

      // if we have isovalues, set these values on the context
      VKLData valuesData;

      if (!params->isoValues.empty()) {
        valuesData = vklNewData(getOpenVKLDevice(),
                                params->isoValues.size(),
                                VKL_FLOAT,
                                params->isoValues.data());
      }

      vklSetData(hitContext, "values", valuesData);

      if (valuesData) {
        vklRelease(valuesData);
      }

      vklCommit(hitContext);

      iteratorSize = vklGetHitIteratorSize(&hitContext);
    }

    void HitIteratorRendererGpu::reallocateBuffers(const size_t width,
                                                   const size_t height)
    {
      deallocateBuffers();

      hitIteratorBuffer = sycl::malloc_device<char>(
          width * height * iteratorSize, getSyclQueue());
      shadowHitIteratorBuffer = sycl::malloc_device<char>(
          width * height * iteratorSize, getSyclQueue());
    }

    void HitIteratorRendererGpu::deallocateBuffers()
    {
      sycl::free(hitIteratorBuffer, getSyclQueue());
      hitIteratorBuffer = nullptr;
      sycl::free(shadowHitIteratorBuffer, getSyclQueue());
      shadowHitIteratorBuffer = nullptr;
    }

    void HitIteratorRendererGpu::onBufferResize(const size_t width,
                                                const size_t height)
    {
      RendererGpu::onBufferResize(width, height);
      reallocateBuffers(width, height);
    }

    void HitIteratorRendererGpu::setKernelObjectAttributes(
        HitIteratorRendererGpuKernel *gpuKernelRenderer)
    {
      gpuKernelRenderer->setObjectAttributes(
          scene.volume.getSampler(), scene.volume.getBounds(), *rendererParams);
    }

    sycl::event HitIteratorRendererGpu::invokeGpuRenderPixel(
        sycl::queue &syclQueue,
        BufferGpu &buffer,
        const Ray *rayBuffer,
        const bool clearFramebuffer)
    {
      const unsigned int width  = buffer.getWidth();
      const unsigned int height = buffer.getHeight();
      auto rgbaBuffer           = buffer.getRgba();
      auto weightBuffer         = buffer.getWeight();

      HitIteratorRendererGpuKernel *gpuKernelRendererDevice =
          prepareGpuKernelObject();

      assert(hitIteratorBuffer);
      assert(shadowHitIteratorBuffer);
      assert(hitContext);
      char *_hitIteratorBuffer                = hitIteratorBuffer;
      char *_shadowHitIteratorBuffer          = shadowHitIteratorBuffer;
      const VKLHitIteratorContext _hitContext = hitContext;
      const size_t _iteratorSize              = iteratorSize;

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
            _hitIteratorBuffer + idx * _iteratorSize,
            _shadowHitIteratorBuffer + idx * _iteratorSize,
            _hitContext,
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
