// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ispc_cpp_interop.h"

// ========================================================================== //
// An enum that represents the different filter types available in vdb volumes.
// ========================================================================== //
enum VKLFilter
#if __cplusplus >= 201103L
: vkl_uint32
#endif
{
  // Only read the voxel the sample position is in, treating it as
  // constant.
  VKL_FILTER_NEAREST = 0,
  // Read the eight voxels surrounding the sample position, and
  // interpolate trilinearly.
  VKL_FILTER_LINEAR = 100,
  // Tricubic interpolation.
  VKL_FILTER_CUBIC = 200,
};


