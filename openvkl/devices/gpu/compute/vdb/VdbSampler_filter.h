// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VdbSampler_nearest.h"
#include "VdbSampler_tricubic.h"
#include "VdbSampler_trilinear.h"

#include "../feature_flags.h"

// These macros centralize filter dispatch for the public API.
// To add a filter, add the enum and than add a case here.
// Then, implement all of the filter-specific functions above.

#define __vkl_switch_sample_filter_ff(FilterEnum, FeatureFlags, F, ...) \
  {                                                                     \
    if (IS_SAMPLE_FILTER_NEAREST(FilterEnum, FeatureFlags)) {           \
      F##Nearest(__VA_ARGS__);                                          \
    } else if (IS_SAMPLE_FILTER_TRILINEAR(FilterEnum, FeatureFlags)) {  \
      F##Trilinear(__VA_ARGS__);                                        \
    } else if (IS_SAMPLE_FILTER_TRICUBIC(FilterEnum, FeatureFlags)) {   \
      F##Tricubic(__VA_ARGS__);                                         \
    } else {                                                            \
      assert(false);                                                    \
    }                                                                   \
  }

#define __vkl_switch_gradient_filter_ff(FilterEnum, FeatureFlags, F, ...) \
  {                                                                       \
    if (IS_GRADIENT_FILTER_NEAREST(FilterEnum, FeatureFlags)) {           \
      F##Nearest(__VA_ARGS__);                                            \
    } else if (IS_GRADIENT_FILTER_TRILINEAR(FilterEnum, FeatureFlags)) {  \
      F##Trilinear(__VA_ARGS__);                                          \
    } else if (IS_GRADIENT_FILTER_TRICUBIC(FilterEnum, FeatureFlags)) {   \
      F##Tricubic(__VA_ARGS__);                                           \
    } else {                                                              \
      assert(false);                                                      \
    }                                                                     \
  }
