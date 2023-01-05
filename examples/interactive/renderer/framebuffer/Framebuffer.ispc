// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <rkcommon/math/vec.ih>

inline vec4f linearToSrgba(const vec4f &rgba)
{
  const uniform float gamma = 1.f / 2.2f;
  return make_vec4f(powf(rgba.x * rgba.w, gamma),
                    powf(rgba.y * rgba.w, gamma),
                    powf(rgba.z * rgba.w, gamma),
                    1.f);
}

export void tonemap(uniform uint32 offset,
                    uniform uint32 size,
                    const float *uniform _weight,
                    const vec4f *uniform _rgba,
                    vec4f *uniform _outputRgba)
{
  const uint32 idx = offset + programIndex;
  if (idx >= size) {
    return;
  }

  vec4f rgba         = _rgba[idx];
  const float weight = _weight[idx];

  if (weight > 0.f) {
    const float rweight = 1.f / weight;
    rgba                = linearToSrgba(rweight * rgba);
  } else {
    rgba = make_vec4f(0.f, 0.f, 0.f, 1.f);
  }

  _outputRgba[idx] = rgba;
}

