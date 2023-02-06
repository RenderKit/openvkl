// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace openvkl {
  namespace examples {

    struct RayMarchIteratorParams
    {
      float intervalResolutionHint{0.5f};
      float samplingRate{1.f};
    };
  }  // namespace examples
}  // namespace openvkl