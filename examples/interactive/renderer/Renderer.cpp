// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.h"

namespace openvkl {
  namespace examples {

    Renderer::Renderer(Scene &scene) : scene(scene), scheduler(scene.scheduler)
    {
      // Note:
      // Do not start the renderer here - the render functions don't exist
      // until the derived classes have been constructed.
      assert(!run.load());
    }

    Renderer::~Renderer()
    {
      // Note:
      // Do not stop the renderer here - the render functions don't exist
      // any more.
      assert(!run.load());
    }

    void Renderer::beforeFrame(bool &needToClear)
    {
      bool cameraChanged = false;
      scheduler.locked(scene.camera, [&]() {
        cameraChanged = cam.updateIfChanged(scene.camera);
      });

      bool rendererParamsChanged = false;
      scheduler.locked(scene.rendererParams, [&]() {
        rendererParamsChanged =
            rendererParams.updateIfChanged(scene.rendererParams);
      });

      needToClear |= cameraChanged || rendererParamsChanged;
    }

    vec4f Renderer::sampleTransferFunction(float value) const
    {
      vec4f colorAndOpacity{0.f};

      const auto &valueRange = rendererParams->transferFunction.valueRange;
      const auto &colorsAndOpacities =
          rendererParams->transferFunction.colorsAndOpacities;

      if (std::isnan(value) || colorsAndOpacities.empty()) {
        return colorAndOpacity;
      }

      if (value <= valueRange.lower) {
        return colorsAndOpacities[0];
      }

      if (value >= valueRange.upper) {
        return colorsAndOpacities.back();
      }

      // map the value into the range [0, size - 1]
      value = (value - valueRange.lower) /
              (valueRange.upper - valueRange.lower) *
              (colorsAndOpacities.size() - 1);

      // index and fractional offset
      const int index       = floor(value);
      const float remainder = value - index;

      // the final interpolated value
      return ((1.f - remainder) * colorsAndOpacities[index] +
              remainder * colorsAndOpacities[min(
                              index + 1, int(colorsAndOpacities.size() - 1))]);
    }

    // -------------------------------------------------------------------------

  }  // namespace examples
}  // namespace openvkl
