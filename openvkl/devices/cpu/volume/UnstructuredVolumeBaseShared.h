// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VolumeShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Node
{
  VKL_INTEROP_UNIFORM vec3f nominalLength;
  VKL_INTEROP_UNIFORM box1f valueRange;
  VKL_INTEROP_UNIFORM int level;
  VKL_INTEROP_UNIFORM Node *VKL_INTEROP_UNIFORM parent;
};

struct VKLUnstructuredBase
{
  VolumeShared super;

  VKL_INTEROP_UNIFORM box3f boundingBox;
  VKL_INTEROP_UNIFORM Node *VKL_INTEROP_UNIFORM bvhRoot;
};

#ifdef __cplusplus
}
#endif // __cplusplus
