// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Ray.h"
#include "Renderer.h"
#include "framebuffer/FramebufferGpu.h"

namespace openvkl {
  namespace examples {

    /*
     * A renderer based on the gpu.
     * This base class expects the renderer to fill the front buffer
     * in renderFrameImpl().
     */
    class RendererGpu : public Renderer
    {
     public:
      RendererGpu(Scene &scene);
      ~RendererGpu();
      void renderFrameImpl(bool clearFramebuffer) override final;
      const Framebuffer &getFramebuffer(size_t w, size_t h) override final;

     protected:
      // This method is responsible for invoke GPU kernel which will
      // render all pixels for given input
      virtual sycl::event invokeGpuRenderPixel(sycl::queue &syclQueue,
                                               BufferGpu &buffer,
                                               const Ray *rayBuffer,
                                               const bool clearFramebuffer) = 0;

     private:
      sycl::queue syclQueue;
      FramebufferGpu framebuffer;

      Ray *rayBuffer{nullptr};
      Ray *rayBufferHost{nullptr};

      void reallocateBuffers(size_t width, size_t height);
      void deallocateBuffers();

      sycl::event invokeGpuRayGeneration(const unsigned int width,
                                         const unsigned int height);
    };

  }  // namespace examples
}  // namespace openvkl
