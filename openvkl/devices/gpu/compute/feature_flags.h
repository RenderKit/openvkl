// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// helpers for feature flag conditions
#define FEATURE_FLAGS_ONLY(WANT_FLAG, NOT_WANT_FLAG1, NOT_WANT_FLAG2) \
  (featureFlags & WANT_FLAG &&                                        \
   !(featureFlags & NOT_WANT_FLAG1 || featureFlags & NOT_WANT_FLAG2))

#define IS_FILTER_NEAREST(FilterType, FilterEnum, FeatureFlags)          \
  FEATURE_FLAGS_ONLY(VKL_FEATURE_FLAG_##FilterType##_FILTER_NEAREST,     \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_TRILINEAR,   \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_TRICUBIC) || \
      (FilterEnum == VKL_FILTER_NEAREST &&                               \
       FeatureFlags & VKL_FEATURE_FLAG_##FilterType##_FILTER_NEAREST)

#define IS_FILTER_TRILINEAR(FilterType, FilterEnum, FeatureFlags)        \
  FEATURE_FLAGS_ONLY(VKL_FEATURE_FLAG_##FilterType##_FILTER_TRILINEAR,   \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_NEAREST,     \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_TRICUBIC) || \
      (FilterEnum == VKL_FILTER_TRILINEAR &&                             \
       FeatureFlags & VKL_FEATURE_FLAG_##FilterType##_FILTER_TRILINEAR)

#define IS_FILTER_TRICUBIC(FilterType, FilterEnum, FeatureFlags)          \
  FEATURE_FLAGS_ONLY(VKL_FEATURE_FLAG_##FilterType##_FILTER_TRICUBIC,     \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_NEAREST,      \
                     VKL_FEATURE_FLAG_##FilterType##_FILTER_TRILINEAR) || \
      (FilterEnum == VKL_FILTER_TRICUBIC &&                               \
       FeatureFlags & VKL_FEATURE_FLAG_##FilterType##_FILTER_TRICUBIC)

// nearest
#define IS_SAMPLE_FILTER_NEAREST(FilterEnum, FeatureFlags) \
  IS_FILTER_NEAREST(SAMPLE, FilterEnum, FeatureFlags)

#define IS_GRADIENT_FILTER_NEAREST(FilterEnum, FeatureFlags) \
  IS_FILTER_NEAREST(GRADIENT, FilterEnum, FeatureFlags)

// trilinear
#define IS_SAMPLE_FILTER_TRILINEAR(FilterEnum, FeatureFlags) \
  IS_FILTER_TRILINEAR(SAMPLE, FilterEnum, FeatureFlags)

#define IS_GRADIENT_FILTER_TRILINEAR(FilterEnum, FeatureFlags) \
  IS_FILTER_TRILINEAR(GRADIENT, FilterEnum, FeatureFlags)

// tricubic
#define IS_SAMPLE_FILTER_TRICUBIC(FilterEnum, FeatureFlags) \
  IS_FILTER_TRICUBIC(SAMPLE, FilterEnum, FeatureFlags)

#define IS_GRADIENT_FILTER_TRICUBIC(FilterEnum, FeatureFlags) \
  IS_FILTER_TRICUBIC(GRADIENT, FilterEnum, FeatureFlags)