// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

typedef enum __VKLFeatureFlags
{
  VKL_FEATURE_FLAG_NONE = 0,

  // volume type
  VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME   = 1 << 0,
  VKL_FEATURE_FLAG_STRUCTURED_SPHERICAL_VOLUME = 1 << 1,
  VKL_FEATURE_FLAG_UNSTRUCTURED_VOLUME         = 1 << 2,
  VKL_FEATURE_FLAG_PARTICLE_VOLUME             = 1 << 3,
  VKL_FEATURE_FLAG_AMR_VOLUME                  = 1 << 4,
  VKL_FEATURE_FLAG_VDB_VOLUME                  = 1 << 5,

  // filter (for sampling)
  VKL_FEATURE_FLAG_SAMPLE_FILTER_NEAREST   = 1 << 6,
  VKL_FEATURE_FLAG_SAMPLE_FILTER_TRILINEAR = 1 << 7,
  VKL_FEATURE_FLAG_SAMPLE_FILTER_TRICUBIC  = 1 << 8,

  // filter (for gradients)
  VKL_FEATURE_FLAG_GRADIENT_FILTER_NEAREST   = 1 << 9,
  VKL_FEATURE_FLAG_GRADIENT_FILTER_TRILINEAR = 1 << 10,
  VKL_FEATURE_FLAG_GRADIENT_FILTER_TRICUBIC  = 1 << 11,

  // temporal formats
  VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_CONSTANT     = 1 << 12,
  VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_STRUCTURED   = 1 << 13,
  VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_UNSTRUCTURED = 1 << 14,

  // for unstructured volumes only
  VKL_FEATURE_FLAG_HAS_CELL_TYPE_TETRAHEDRON = 1 << 15,
  VKL_FEATURE_FLAG_HAS_CELL_TYPE_HEXAHEDRON  = 1 << 16,
  VKL_FEATURE_FLAG_HAS_CELL_TYPE_WEDGE       = 1 << 17,
  VKL_FEATURE_FLAG_HAS_CELL_TYPE_PYRAMID     = 1 << 18,

  VKL_FEATURE_FLAG_ALL = 0xffffffff,
} VKLFeatureFlags;

#ifdef __cplusplus

// convenience oeprators for working with the above
inline VKLFeatureFlags operator|(VKLFeatureFlags a, VKLFeatureFlags b)
{
  return static_cast<VKLFeatureFlags>(static_cast<int>(a) |
                                      static_cast<int>(b));
}

inline VKLFeatureFlags &operator|=(VKLFeatureFlags &a, VKLFeatureFlags b)
{
  return reinterpret_cast<VKLFeatureFlags &>(reinterpret_cast<int &>(a) |=
                                             static_cast<int>(b));
}

#endif
