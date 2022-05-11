// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ispc_cpp_interop.h"

// ========================================================================== //
// An enum for data format constants.
// This value determines how voxel data buffers are interpreted by VKL.
// ========================================================================== //
#if __cplusplus > 201103L
enum VKLFormat : vkl_uint32
#else
enum VKLFormat
#endif
{
  // The node has no spatial variation.
  VKL_FORMAT_TILE = 0,
  // The buffer contains a dense grid of voxels.
  // The suffix _ZYX indicates z-major ordering, i.e., the z-coordinate
  // advances most quickly.
  VKL_FORMAT_DENSE_ZYX,
  VKL_FORMAT_INVALID = 100
};


