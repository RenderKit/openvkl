// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#define M_PI_FLOAT M_PI ## f

namespace ispc {

  inline void transformObjectToLocal_uniform_structured_regular(
      const SharedStructuredVolume *self,
      const vec3f &objectCoordinates,
      vec3f &localCoordinates)
  {
    localCoordinates =
        1.f / self->gridSpacing * (objectCoordinates - self->gridOrigin);
  }

  inline void transformObjectToLocal_uniform_structured_spherical(
      const SharedStructuredVolume *self,
      const vec3f &objectCoordinates,
      vec3f &localCoordinates)
  {
    /* (x, y, z) -> (r, inclination, azimuth), using the ISO convention for
     coordinates and ordering. all angles in radians. */
    const float r = sqrtf(objectCoordinates.x * objectCoordinates.x +
                          objectCoordinates.y * objectCoordinates.y +
                          objectCoordinates.z * objectCoordinates.z);

    const float inclination = acos(objectCoordinates.z / r);

    float azimuth = atan2(objectCoordinates.y, objectCoordinates.x);

    /* the above returns [-PI, PI], while our azimuth grid convention is [0,
     * 2*PI] */
    if (azimuth < 0.f) {
      azimuth += 2.f * M_PI_FLOAT;
    }

    localCoordinates.x = (1.f / self->gridSpacing.x) * (r - self->gridOrigin.x);
    localCoordinates.y =
        (1.f / self->gridSpacing.y) * (inclination - self->gridOrigin.y);
    localCoordinates.z =
        (1.f / self->gridSpacing.z) * (azimuth - self->gridOrigin.z);
  }

  inline void transformObjectToLocal_uniform_dispatch(
      const SharedStructuredVolume *self,
      const vec3f &objectCoordinates,
      vec3f &localCoordinates)
  {
    if (self->gridType == structured_regular) {
      transformObjectToLocal_uniform_structured_regular(
          self, objectCoordinates, localCoordinates);
    } else {
      transformObjectToLocal_uniform_structured_spherical(
          self, objectCoordinates, localCoordinates);
    }
  }
}  // namespace ispc
