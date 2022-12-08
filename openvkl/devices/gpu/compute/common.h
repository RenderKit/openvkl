// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

inline vec3f make_vec3f_rkcommon(const ispc::vec3f &data)
{
  return *reinterpret_cast<vec3f *>((void *)&data);
}

inline void transformObjectToLocal_uniform_structured_regular(
    const ispc::SharedStructuredVolume *self,
    const vec3f &objectCoordinates,
    vec3f &localCoordinates)
{
  localCoordinates =
      1.f / make_vec3f_rkcommon(self->gridSpacing) *
      (objectCoordinates - make_vec3f_rkcommon(self->gridOrigin));
}
