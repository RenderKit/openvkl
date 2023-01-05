// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../RendererGpu.h"
#include "../Random.h"
#include "DensityPathTracerGpuKernel.h"
#include "DensityPathTracerParams.h"

namespace openvkl {
  namespace examples {

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class DensityPathTracerGpu final : public RendererGpu
    {
     public:
      DensityPathTracerGpu(Scene &scene);
      ~DensityPathTracerGpu();
      void beforeFrame(bool &needToClear) override final;
      void afterFrame() override final;

      Versioned<DensityPathTracerParams> &getGuiParams()
      {
        return guiParams;
      }

     protected:
      sycl::event invokeGpuRenderPixel(
          sycl::queue &syclQueue,
          BufferGpu &buffer,
          const Ray *rayBuffer,
          const bool clearFramebuffer) override final;

     private:
      Versioned<DensityPathTracerParams> guiParams;
      Versioned<DensityPathTracerParams> params;  // Used by the worker.
      size_t frameId{0};  // This is used for random sampling.

      DensityPathTracerGpuKernel *gpuKernelRendererDevice{nullptr};
      DensityPathTracerGpuKernel *gpuKernelRendererHost{nullptr};
    };

  }  // namespace examples
}  // namespace openvkl
