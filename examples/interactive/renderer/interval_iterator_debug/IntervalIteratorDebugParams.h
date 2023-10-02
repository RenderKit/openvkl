// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace openvkl {
  namespace examples {

    struct IntervalIteratorDebugParams
    {
      float intervalResolutionHint{0.5f};
      float intervalColorScale{4.f};
      float intervalOpacity{0.25f};
      bool firstIntervalOnly{false};
      bool showIntervalBorders{false};
    };
  }  // namespace examples
}  // namespace openvkl