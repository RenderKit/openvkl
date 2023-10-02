// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../../cpu/volume/amr/AMRShared.h"

namespace ispc {

  inline vec3f lerp(const box3f box, const vec3f w)
  {
    return box.lower + (box.upper - box.lower) * w;
  }

  inline float max(float a, float b, float c)
  {
    return max(max(a, b), c);
  }

  inline float min(float a, float b, float c)
  {
    return min(min(a, b), c);
  }

  inline float max(
      float a, float b, float c, float d, float e, float f, float g, float h)
  {
    return max(max(max(a, b), max(c, d)), max(max(e, f), max(g, h)));
  }

  inline float min(
      float a, float b, float c, float d, float e, float f, float g, float h)
  {
    return min(min(min(a, b), min(c, d)), min(min(e, f), min(g, h)));
  }

  inline vec3f nextafter(const vec3f v, const vec3f sign)
  {
    return make_vec3f(
        nextafter(v.x, sign.x), nextafter(v.y, sign.y), nextafter(v.z, sign.z));
  }

}  // namespace ispc
