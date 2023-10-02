// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Random.h"
#include "../RendererGpu.h"
#include "DensityPathTracerGpuKernel.h"
#include "DensityPathTracerParams.h"

namespace openvkl {
  namespace examples {

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class DensityPathTracerGpu final
        : public RendererGpu<DensityPathTracerGpuKernel, DensityPathTracerParams>
    {
     public:
      DensityPathTracerGpu(Scene &scene);
      ~DensityPathTracerGpu();
      void beforeFrame(bool &needToClear) override final;

     protected:
      sycl::event invokeGpuRenderPixel(
          sycl::queue &syclQueue,
          BufferGpu &buffer,
          const Ray *rayBuffer,
          const bool clearFramebuffer) override final;

     private:
      void setKernelObjectAttributes(
          DensityPathTracerGpuKernel *gpuKernelRenderer) override final;
    };

  }  // namespace examples
}  // namespace openvkl
