// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <vector>

namespace openvkl {
  namespace examples {

    struct HitIteratorRendererParams
    {
      std::vector<float> isoValues{-1.f, 0.f, 1.f};
    };
  }  // namespace examples
}  // namespace openvkl