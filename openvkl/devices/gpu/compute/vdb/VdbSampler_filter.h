// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VdbSampler_nearest.h"
#include "VdbSampler_tricubic.h"
#include "VdbSampler_trilinear.h"

// This macro centralizes filter dispatch for the public API.
// To add a filter, add the enum and than add a case here.
// Then, implement all of the filter-specific functions above.
#define __vkl_switch_filter(FilterEnum, F, ...) \
  {                                             \
    switch (FilterEnum) {                       \
    case VKL_FILTER_TRILINEAR: {                \
      F##Trilinear(__VA_ARGS__);                \
      break;                                    \
    }                                           \
    case VKL_FILTER_TRICUBIC: {                 \
      F##Tricubic(__VA_ARGS__);                 \
      break;                                    \
    }                                           \
    default: {                                  \
      assert(FilterEnum == VKL_FILTER_NEAREST); \
      F##Nearest(__VA_ARGS__);                  \
      break;                                    \
    }                                           \
    }                                           \
  }
