// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

struct ValueRanges
{
  int numRanges;
  box1f *ranges;
  box1f rangesMinMax;
};

#ifdef __cplusplus
}
#endif  // __cplusplus

