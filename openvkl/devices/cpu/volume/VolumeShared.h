// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/ispc_cpp_interop.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

#ifndef __ISPC_ENUM_DeviceVolumeType__
#define __ISPC_ENUM_DeviceVolumeType__

  enum DeviceVolumeType
  {
    VOLUME_TYPE_UNKNOWN =
        0,  // must be zero, as memset() will be used for initialization
    VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY,
    VOLUME_TYPE_STRUCTURED_REGULAR,
    VOLUME_TYPE_STRUCTURED_SPHERICAL,
    VOLUME_TYPE_VDB,
    VOLUME_TYPE_AMR,
    VOLUME_TYPE_UNSTRUCTURED,
    VOLUME_TYPE_PARTICLE
  };

#endif

// this hack prevents .ispc -> ispc.h code from redeclaring. TODO: something
// better
#ifndef __ISPC_STRUCT_VolumeShared__
#define __ISPC_STRUCT_VolumeShared__

  struct VolumeShared
  {
    DeviceVolumeType type;

    // Background value, per attribute.
    const float *VKL_INTEROP_UNIFORM background;
  };

#endif

#ifdef __cplusplus
}
#endif  // __cplusplus
