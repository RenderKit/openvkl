// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// helpers for feature flag conditions
#define FEATURE_FLAGS_ONLY(WANT_FLAG, NOT_WANT_FLAG1, NOT_WANT_FLAG2) \
  (featureFlags & WANT_FLAG &&                                        \
   !(featureFlags & NOT_WANT_FLAG1 || featureFlags & NOT_WANT_FLAG2))

#define IS_FILTER_NEAREST(FilterType, FilterEnum, FeatureFlags)       \
  FEATURE_FLAGS_ONLY(VKL_FEATURE_FLAG_##FilterType##_FILTER_NEAREST,  \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_LINEAR,   \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_CUBIC) || \
      (FilterEnum == VKL_FILTER_NEAREST &&                            \
       FeatureFlags & VKL_FEATURE_FLAG_##FilterType##_FILTER_NEAREST)

#define IS_FILTER_LINEAR(FilterType, FilterEnum, FeatureFlags)        \
  FEATURE_FLAGS_ONLY(VKL_FEATURE_FLAG_##FilterType##_FILTER_LINEAR,   \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_NEAREST,  \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_CUBIC) || \
      (FilterEnum == VKL_FILTER_LINEAR &&                             \
       FeatureFlags & VKL_FEATURE_FLAG_##FilterType##_FILTER_LINEAR)

#define IS_FILTER_CUBIC(FilterType, FilterEnum, FeatureFlags)          \
  FEATURE_FLAGS_ONLY(VKL_FEATURE_FLAG_##FilterType##_FILTER_CUBIC,     \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_NEAREST,   \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_LINEAR) || \
      (FilterEnum == VKL_FILTER_CUBIC &&                               \
       FeatureFlags & VKL_FEATURE_FLAG_##FilterType##_FILTER_CUBIC)

// nearest
#define IS_SAMPLE_FILTER_NEAREST(FilterEnum, FeatureFlags) \
  IS_FILTER_NEAREST(SAMPLE, FilterEnum, FeatureFlags)

#define IS_GRADIENT_FILTER_NEAREST(FilterEnum, FeatureFlags) \
  IS_FILTER_NEAREST(GRADIENT, FilterEnum, FeatureFlags)

// trilinear
#define IS_SAMPLE_FILTER_LINEAR(FilterEnum, FeatureFlags) \
  IS_FILTER_LINEAR(SAMPLE, FilterEnum, FeatureFlags)

#define IS_GRADIENT_FILTER_LINEAR(FilterEnum, FeatureFlags) \
  IS_FILTER_LINEAR(GRADIENT, FilterEnum, FeatureFlags)

// tricubic
#define IS_SAMPLE_FILTER_CUBIC(FilterEnum, FeatureFlags) \
  IS_FILTER_CUBIC(SAMPLE, FilterEnum, FeatureFlags)

#define IS_GRADIENT_FILTER_CUBIC(FilterEnum, FeatureFlags) \
  IS_FILTER_CUBIC(GRADIENT, FilterEnum, FeatureFlags)