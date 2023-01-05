// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace openvkl {
  namespace examples {

    struct TransferFunctionShared
    {
      vec4f colorsAndOpacities[TRANSFER_FUNCTION_DEFAULT_NUM_SAMPLES];
      unsigned int numValues = 0;
      range1f valueRange;
    };

    struct RendererParamsShared
    {
      int attributeIndex;
      float time;
      bool fixedFramebufferSize;
      bool restrictPixelRange;
      box2i pixelRange;
      TransferFunctionShared transferFunction;
    };

  }  // namespace examples
}  // namespace openvkl