// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Ray.h"
#include "Renderer.h"
#include "RendererGpuKernel.h"
#include "framebuffer/FramebufferGpu.h"

namespace openvkl {
  namespace examples {

    /*
     * A renderer based on the gpu.
     * This base class expects the renderer to fill the front buffer
     * in renderFrameImpl().
     */
    template <typename TRendererGpuKernelSubtype, typename TRenderParamsType>
    class RendererGpu : public Renderer
    {
     public:
      RendererGpu(Scene &scene);
      ~RendererGpu();
      void renderFrameImpl(bool clearFramebuffer) override final;
      const Framebuffer &getFramebuffer(size_t w, size_t h) override final;
      Versioned<TRenderParamsType> &getGuiParams()
      {
        return guiParams;
      }

     protected:
      size_t frameId{0};
      void afterFrame() override;
      virtual void setKernelObjectAttributes(
          TRendererGpuKernelSubtype *gpuKernelRenderer) = 0;

      // This method is responsible for invoke GPU kernel which will
      // render all pixels for given input
      virtual sycl::event invokeGpuRenderPixel(sycl::queue &syclQueue,
                                               BufferGpu &buffer,
                                               const Ray *rayBuffer,
                                               const bool clearFramebuffer) = 0;

      TRendererGpuKernelSubtype *prepareGpuKernelObject()
      {
        // We need to make sure that we class of the object we want to copy
        // is trivially copyable since we're using memcpy for that.
        static_assert(std::is_trivially_copyable<TRendererGpuKernelSubtype>());

        // Possible optimization - not all renderers have these params
        // changed for each frame so we don't need to copy this object
        // on each frame.

        // Each child needs to implement for it's own params set
        setKernelObjectAttributes(gpuKernelRendererHost);

        // Copy host object to device
        syclQueue.memcpy(gpuKernelRendererDevice,
                         gpuKernelRendererHost,
                         sizeof(TRendererGpuKernelSubtype));
        return gpuKernelRendererDevice;
      }

      virtual void onBufferResize(const size_t width, const size_t height);

      Versioned<TRenderParamsType> guiParams;
      Versioned<TRenderParamsType> params;  // Used by the worker.


     private:
      TRendererGpuKernelSubtype *gpuKernelRendererHost{nullptr};
      TRendererGpuKernelSubtype *gpuKernelRendererDevice{nullptr};
      sycl::queue syclQueue;
      FramebufferGpu framebuffer;

      Ray *rayBuffer{nullptr};
      Ray *rayBufferHost{nullptr};

      sycl::event invokeGpuRayGeneration(const unsigned int width,
                                         const unsigned int height);

      void reallocateBuffers(size_t width, size_t height);
      void deallocateBuffers();
    };

  }  // namespace examples
}  // namespace openvkl
