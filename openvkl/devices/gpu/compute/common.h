// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rkcommon/math/math.ih>

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

inline void transformObjectToLocal_uniform_structured_spherical(
    const ispc::SharedStructuredVolume *self,
    const vec3f &objectCoordinates,
    vec3f &localCoordinates)
{
  /* (x, y, z) -> (r, inclination, azimuth), using the ISO convention for
   coordinates and ordering. all angles in radians. */
  const float r = ispc::sqrtf(objectCoordinates.x * objectCoordinates.x +
                              objectCoordinates.y * objectCoordinates.y +
                              objectCoordinates.z * objectCoordinates.z);

  const float inclination = ispc::acos(objectCoordinates.z / r);

  float azimuth = ispc::atan2(objectCoordinates.y, objectCoordinates.x);

  /* the above returns [-PI, PI], while our azimuth grid convention is [0,
   * 2*PI] */
  if (azimuth < 0.f) {
    azimuth += 2.f * M_PI;
  }

  localCoordinates.x = (1.f / self->gridSpacing.x) * (r - self->gridOrigin.x);
  localCoordinates.y =
      (1.f / self->gridSpacing.y) * (inclination - self->gridOrigin.y);
  localCoordinates.z =
      (1.f / self->gridSpacing.z) * (azimuth - self->gridOrigin.z);
}

inline void transformObjectToLocal_uniform_dispatch(
    const ispc::SharedStructuredVolume *self,
    const vec3f &objectCoordinates,
    vec3f &localCoordinates)
{
  if (self->gridType == ispc::structured_regular) {
    transformObjectToLocal_uniform_structured_regular(
        self, objectCoordinates, localCoordinates);
  } else {
    transformObjectToLocal_uniform_structured_spherical(
        self, objectCoordinates, localCoordinates);
  }
}
