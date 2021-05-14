// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ispc_cpp_interop.h"

// ========================================================================== //
// An enum for temporal format constants.
// ========================================================================== //
#if __cplusplus > 201103L
enum VKLTemporalFormat : vkl_uint32
#else
enum VKLTemporalFormat
#endif
{
  // There is no temporal variation.
  VKL_TEMPORAL_FORMAT_CONSTANT = 0,
  // Each voxel contains the same number of temporal samples and they
  // are placed regularly in [0, 1].
  VKL_TEMPORAL_FORMAT_STRUCTURED,
  // Temporal resolution is adaptive, such that voxels may have different 
  // numbers of time steps. See the api documentation for more detail.
  VKL_TEMPORAL_FORMAT_UNSTRUCTURED,
  VKL_TEMPORAL_FORMAT_INVALID = 100
};


