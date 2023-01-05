// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Random.h"
#include "../RendererHost.h"
#include "DensityPathTracerParams.h"

namespace openvkl {
  namespace examples {

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class DensityPathTracerIspc final : public IspcRenderer
    {
     public:
      DensityPathTracerIspc(Scene &scene);
      ~DensityPathTracerIspc();
      void beforeFrame(bool &needToClear) override final;
      void afterFrame() override final;

      Versioned<DensityPathTracerParams> &getGuiParams() {
        return guiParams;
      }

     protected:
      void renderPixelBlock(const vec2i &resolution,
                            uint32_t offset,
                            vec4f *rgbas,
                            float *weights) const override final;

     private:
      Versioned<DensityPathTracerParams> guiParams;
      Versioned<DensityPathTracerParams> params;  // Used by the worker.
      void *ispcParams{nullptr};
      size_t frame{0};  // This is used for random sampling.
    };

  }  // namespace examples
}  // namespace openvkl
