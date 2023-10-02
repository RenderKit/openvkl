// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace openvkl {
  namespace examples {

    struct DensityPathTracerParams
    {
      float shutter{0.5f};
      bool motionBlur{false};
      float sigmaTScale{1.f};
      float sigmaSScale{1.f};
      int maxNumScatters{1};
      float ambientLightIntensity{1.f};
      bool showBbox{false};
    };
  }  // namespace examples
}  // namespace openvkl