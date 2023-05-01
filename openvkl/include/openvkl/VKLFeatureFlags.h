// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

typedef enum __VKLFeatureFlags
{
  VKL_FEATURE_FLAG_NONE = 0,

  VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME   = 1 << 0,
  VKL_FEATURE_FLAG_STRUCTURED_SPHERICAL_VOLUME = 1 << 1,
  VKL_FEATURE_FLAG_UNSTRUCTURED_VOLUME         = 1 << 2,
  VKL_FEATURE_FLAG_PARTICLE_VOLUME             = 1 << 3,
  VKL_FEATURE_FLAG_AMR_VOLUME                  = 1 << 4,
  VKL_FEATURE_FLAG_VDB_VOLUME                  = 1 << 5,

  VKL_FEATURE_FLAG_ALL = 0xffffffff,
} VKLFeatureFlags;
