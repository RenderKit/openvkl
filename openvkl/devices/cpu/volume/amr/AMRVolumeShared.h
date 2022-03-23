// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "AMRShared.h"
#include "../UnstructuredVolumeBaseShared.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  struct AMRVolume
  {
    VKLUnstructuredBase super;

    VKL_INTEROP_UNIFORM vec3f gridSpacing;
    VKL_INTEROP_UNIFORM vec3f gridOrigin;
    VKL_INTEROP_UNIFORM box3f boundingBox;
    VKL_INTEROP_UNIFORM float samplingStep;

    AMR amr;
  };

#ifdef __cplusplus
}
#endif  // __cplusplus
