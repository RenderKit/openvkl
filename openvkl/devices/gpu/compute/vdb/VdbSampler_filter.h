// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VdbSampler_nearest.h"
#include "VdbSampler_tricubic.h"
#include "VdbSampler_trilinear.h"

// These macros centralize filter dispatch for the public API.
// To add a filter, add the enum and than add a case here.
// Then, implement all of the filter-specific functions above.

#define __vkl_switch_sample_filter_ff(FilterEnum, FeatureFlags, F, ...)   \
  {                                                                       \
    if (FilterEnum == VKL_FILTER_NEAREST &&                               \
        featureFlags & VKL_FEATURE_FLAG_SAMPLE_FILTER_NEAREST) {          \
      F##Nearest(__VA_ARGS__);                                            \
    } else if (FilterEnum == VKL_FILTER_TRILINEAR &&                      \
               featureFlags & VKL_FEATURE_FLAG_SAMPLE_FILTER_TRILINEAR) { \
      F##Trilinear(__VA_ARGS__);                                          \
    } else if (FilterEnum == VKL_FILTER_TRICUBIC &&                       \
               featureFlags & VKL_FEATURE_FLAG_SAMPLE_FILTER_TRICUBIC) {  \
      F##Tricubic(__VA_ARGS__);                                           \
    } else {                                                              \
      assert(false);                                                      \
    }                                                                     \
  }

#define __vkl_switch_gradient_filter_ff(FilterEnum, FeatureFlags, F, ...)   \
  {                                                                         \
    if (FilterEnum == VKL_FILTER_NEAREST &&                                 \
        featureFlags & VKL_FEATURE_FLAG_GRADIENT_FILTER_NEAREST) {          \
      F##Nearest(__VA_ARGS__);                                              \
    } else if (FilterEnum == VKL_FILTER_TRILINEAR &&                        \
               featureFlags & VKL_FEATURE_FLAG_GRADIENT_FILTER_TRILINEAR) { \
      F##Trilinear(__VA_ARGS__);                                            \
    } else if (FilterEnum == VKL_FILTER_TRICUBIC &&                         \
               featureFlags & VKL_FEATURE_FLAG_GRADIENT_FILTER_TRICUBIC) {  \
      F##Tricubic(__VA_ARGS__);                                             \
    } else {                                                                \
      assert(false);                                                        \
    }                                                                       \
  }
