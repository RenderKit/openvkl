// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VdbSampler_packed_denseZYX.h"
#include "VdbSampler_packed_tile.h"

// ---------------------------------------------------------------------------
// Helper that generates code to dispatch to a leaf handler implementation based
// on the attribute data type and leaf format.
// ---------------------------------------------------------------------------

#define __vkl_vdb_packed_handler_impl(handler, postfix, ...) \
  handler##_##postfix(__VA_ARGS__);

#define __vkl_vdb_packed_handler_format(handler, leafFormat, postfix, ...)     \
  switch (leafFormat) {                                                        \
  case VKL_FORMAT_TILE: {                                                      \
    __vkl_vdb_packed_handler_impl(handler, tile_##postfix, __VA_ARGS__) break; \
  }                                                                            \
  case VKL_FORMAT_DENSE_ZYX: {                                                 \
    __vkl_vdb_packed_handler_impl(                                             \
        handler, denseZYX_##postfix, __VA_ARGS__) break;                       \
  }                                                                            \
  default:                                                                     \
    assert(false);                                                             \
    break;                                                                     \
  }

#define __vkl_vdb_packed_handler_dataType(                 \
    handler, dataType, leafFormat, postfix, ...)           \
  /* Not currently supported on GPU:                       \
  if (dataType == VKL_HALF) {                              \
    __vkl_vdb_packed_handler_format(                       \
        handler, leafFormat, half_##postfix, __VA_ARGS__)  \
  } else*/                                                 \
  if (dataType == VKL_FLOAT) {                             \
    __vkl_vdb_packed_handler_format(                       \
        handler, leafFormat, float_##postfix, __VA_ARGS__) \
  } else {                                                 \
    assert(false);                                         \
  }

#define __vkl_vdb_packed_handler(                       \
    handler, addressing32, dataType, leafFormat, ...)   \
  if (addressing32) {                                   \
    __vkl_vdb_packed_handler_dataType(                  \
        handler, dataType, leafFormat, 32, __VA_ARGS__) \
  } else {                                              \
    __vkl_vdb_packed_handler_dataType(                  \
        handler, dataType, leafFormat, 64, __VA_ARGS__) \
  }
