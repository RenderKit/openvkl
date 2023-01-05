// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <rkcommon/math/box.h>
#include <rkcommon/tasking/parallel_for.h>
#include <chrono>

#include "DensityPathTracerIspc.h"
#include "DensityPathTracer_ispc.h"

namespace openvkl {
  namespace examples {

    DensityPathTracerIspc::DensityPathTracerIspc(Scene &scene)
        : IspcRenderer{scene}
    {
      ispcParams = ispc::DensityPathTracerParams_create();
    }

    DensityPathTracerIspc::~DensityPathTracerIspc()
    {
      scheduler.stop(*this);
      ispc::DensityPathTracerParams_destroy(ispcParams);
      ispcParams = nullptr;
    }

    void DensityPathTracerIspc::beforeFrame(bool &needToClear)
    {
      IspcRenderer::beforeFrame(needToClear);

      scheduler.locked(guiParams, [&]() {
        needToClear |= params.updateIfChanged(guiParams);
      });

      if (needToClear) {
        ispc::DensityPathTracerParams_set(ispcParams,
                                          params->shutter,
                                          params->motionBlur,
                                          params->sigmaTScale,
                                          params->sigmaSScale,
                                          params->maxNumScatters,
                                          params->ambientLightIntensity,
                                          params->showBbox);
      }
    }

    void DensityPathTracerIspc::afterFrame()
    {
      ++frame;
    }

    void DensityPathTracerIspc::renderPixelBlock(const vec2i &resolution,
                                                 uint32_t offset,
                                                 vec4f *rgbas,
                                                 float *weights) const
    {
      ispc::DensityPathTracer_renderPixel(
          ispcParams,
          ispcScene,
          static_cast<uint32_t>(frame),
          reinterpret_cast<const ispc::vec2i &>(resolution),
          offset,
          reinterpret_cast<ispc::vec4f *>(rgbas),
          weights);
    }

  }  // namespace examples
}  // namespace openvkl
