// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../RendererGpu.h"
#include "HitIteratorRendererGpuKernel.h"
#include "HitIteratorRendererParams.h"

namespace openvkl {
  namespace examples {

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class HitIteratorRendererGpu final
        : public RendererGpu<HitIteratorRendererGpuKernel, HitIteratorRendererParams>
    {
     public:
      HitIteratorRendererGpu(Scene &scene);
      ~HitIteratorRendererGpu();
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
          HitIteratorRendererGpuKernel *gpuKernelRenderer) override final;
      void updateHitContext();
      void reallocateBuffers(const size_t width, const size_t height);
      void deallocateBuffers();
      char *hitIteratorBuffer{nullptr};
      char *shadowHitIteratorBuffer{nullptr};


      VKLHitIteratorContext hitContext;
      size_t iteratorSize{0};
    };

  }  // namespace examples
}  // namespace openvkl
