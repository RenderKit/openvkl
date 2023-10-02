// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VdbSampler_denseZYX.h"
#include "VdbSampler_tile.h"

// ---------------------------------------------------------------------------
// Helper that generates code to dispatch to a leaf handler implementation based
// on the attribute data type, format, and temporal format.
// ---------------------------------------------------------------------------

#define __vkl_vdb_leaf_handler_impl(handler, postfix, ...) \
  handler##_##postfix(__VA_ARGS__);

#define __vkl_vdb_leaf_handler_format(handler, leafFormat, postfix, ...)     \
  switch (leafFormat) {                                                      \
  case VKL_FORMAT_TILE: {                                                    \
    __vkl_vdb_leaf_handler_impl(handler, tile_##postfix, __VA_ARGS__) break; \
  }                                                                          \
  case VKL_FORMAT_DENSE_ZYX: {                                               \
    __vkl_vdb_leaf_handler_impl(                                             \
        handler, denseZYX_##postfix, __VA_ARGS__) break;                     \
  }                                                                          \
  default:                                                                   \
    assert(false);                                                           \
    break;                                                                   \
  }

#define __vkl_vdb_leaf_handler_temporalFormat(handler,                         \
                                              featureFlags,                    \
                                              leafFormat,                      \
                                              allLeavesConstant,               \
                                              leafTemporalFormat,              \
                                              postfix,                         \
                                              ...)                             \
  if ((featureFlags & VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_CONSTANT &&         \
       !(featureFlags & VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_STRUCTURED ||     \
         featureFlags & VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_UNSTRUCTURED)) || \
      allLeavesConstant ||                                                     \
      leafTemporalFormat == VKL_TEMPORAL_FORMAT_CONSTANT) {                    \
    __vkl_vdb_leaf_handler_format(                                             \
        handler, leafFormat, constant_##postfix, __VA_ARGS__);                 \
                                                                               \
  } else if ((featureFlags &                                                   \
                  VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_STRUCTURED &&           \
              !(featureFlags &                                                 \
                    VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_CONSTANT ||           \
                featureFlags &                                                 \
                    VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_UNSTRUCTURED)) ||     \
             leafTemporalFormat == VKL_TEMPORAL_FORMAT_STRUCTURED) {           \
    __vkl_vdb_leaf_handler_format(                                             \
        handler, leafFormat, structured_##postfix, __VA_ARGS__);               \
                                                                               \
  } else if ((featureFlags &                                                   \
                  VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_UNSTRUCTURED &&         \
              !(featureFlags &                                                 \
                    VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_CONSTANT ||           \
                featureFlags &                                                 \
                    VKL_FEATURE_FLAG_HAS_TEMPORAL_FORMAT_STRUCTURED)) ||       \
             leafTemporalFormat == VKL_TEMPORAL_FORMAT_UNSTRUCTURED) {         \
    __vkl_vdb_leaf_handler_format(                                             \
        handler, leafFormat, unstructured_##postfix, __VA_ARGS__);             \
  }                                                                            \
                                                                               \
  else {                                                                       \
    assert(false);                                                             \
  }

#define __vkl_vdb_leaf_handler(handler,                       \
                               featureFlags,                  \
                               dataType,                      \
                               leafFormat,                    \
                               allLeavesConstant,             \
                               leafTemporalFormat,            \
                               ...)                           \
  /* Not currently supported on GPU:                          \
    if (dataType == VKL_HALF) {                               \
    __vkl_vdb_leaf_handler_temporalFormat(handler,            \
                                          leafFormat,         \
                                          allLeavesConstant,  \
                                          leafTemporalFormat, \
                                          half,               \
                                          __VA_ARGS__)        \
  } else*/                                                    \
  assert(dataType == VKL_FLOAT);                              \
  __vkl_vdb_leaf_handler_temporalFormat(handler,              \
                                        featureFlags,         \
                                        leafFormat,           \
                                        allLeavesConstant,    \
                                        leafTemporalFormat,   \
                                        float,                \
                                        __VA_ARGS__)
