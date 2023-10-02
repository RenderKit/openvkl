// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

#ifndef __ISPC_STRUCT_ValueRanges__
#define __ISPC_STRUCT_ValueRanges__
  struct ValueRanges
  {
    int numRanges;
    box1f *ranges;
    box1f rangesMinMax;
  };
#endif

#ifdef __cplusplus
}
#endif  // __cplusplus
