// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"

namespace openvkl {
  namespace examples {

    using namespace rkcommon::math;

    struct Ray
    {
      vec3f org;
      vec3f dir;
      range1f t;
    };

  }  // namespace examples
}  // namespace openvkl
