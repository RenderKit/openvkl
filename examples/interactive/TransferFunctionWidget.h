// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>  // Push/PopDisabled

#include <rkcommon/math/range.h>
#include <rkcommon/math/vec.h>

#include <functional>
#include <string>
#include <vector>

namespace openvkl {
  namespace examples {

    using namespace rkcommon::math;

    using ColorPoint   = vec4f;
    using OpacityPoint = vec2f;

    class TransferFunctionWidget
    {
     public:
      TransferFunctionWidget(
          const range1f &valueRange     = range1f(-1.f, 1.f),
          const std::string &widgetName = "Transfer Function");
      ~TransferFunctionWidget();

      // update UI and process any UI events
      // Returns true if anything was changed.
      bool updateUI();

      // getters for current transfer function data
      range1f getValueRange() const;
      void setValueRange(const range1f &range);
      std::vector<vec4f> getSampledColorsAndOpacities(int numSamples = 256)
        const;

     private:
      void loadDefaultMaps();
      void setMap(int);

      vec3f interpolateColor(const std::vector<ColorPoint> &controlPoints,
                             float x) const;

      float interpolateOpacity(const std::vector<OpacityPoint> &controlPoints,
                               float x) const;

      void updateTfnPaletteTexture();

      void drawEditor();

      // all available transfer functions
      std::vector<std::string> tfnsNames;
      std::vector<std::vector<ColorPoint>> tfnsColorPoints;
      std::vector<std::vector<OpacityPoint>> tfnsOpacityPoints;
      std::vector<bool> tfnsEditable;

      // properties of currently selected transfer function
      int currentMap{0};
      std::vector<ColorPoint> *tfnColorPoints{nullptr};
      std::vector<OpacityPoint> *tfnOpacityPoints{nullptr};
      bool tfnEditable{true};

      // flag indicating transfer function has changed in UI
      bool tfnChanged{true};

      // scaling factor for generated opacities
      float globalOpacityScale{1.f};

      // domain (value range) of transfer function
      range1f valueRange{-1.f, 1.f};

      // texture for displaying transfer function color palette
      GLuint tfnPaletteTexture{0};

      // widget name (use different names to support multiple concurrent
      // widgets)
      std::string widgetName;
    };

  }  // namespace examples
}  // namespace openvkl
