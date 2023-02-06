// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../RendererGpu.h"
#include "RayMarchIteratorGpuKernel.h"

namespace openvkl {
  namespace examples {

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class RayMarchIteratorGpu final
        : public RendererGpu<RayMarchIteratorGpuKernel, RayMarchIteratorParams>
    {
     public:
      RayMarchIteratorGpu(Scene &scene);
      ~RayMarchIteratorGpu();
      void beforeFrame(bool &needToClear) override final;

     protected:
      sycl::event invokeGpuRenderPixel(
          sycl::queue &syclQueue,
          BufferGpu &buffer,
          const Ray *rayBuffer,
          const bool clearFramebuffer) override final;

     private:
      void onBufferResize(const size_t width,
                          const size_t height) override final;
      void setKernelObjectAttributes(
          RayMarchIteratorGpuKernel *gpuKernelRenderer) override final;
      void updateIntervalContext();
      void reallocateBuffers(const size_t width, const size_t height);
      void deallocateBuffers();

      VKLIntervalIteratorContext intervalContext;
      char *iteratorBuffer{nullptr};
      size_t iteratorSize{0};
    };

  }  // namespace examples
}  // namespace openvkl
