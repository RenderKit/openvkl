// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/ispc_cpp_interop.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  struct IntervalIteratorContext;
  struct HitIteratorContext;

#ifndef __ISPC_STRUCT_IntervalIteratorShared__
#define __ISPC_STRUCT_IntervalIteratorShared__
  // must be first member of interval iterator structs
  struct IntervalIteratorShared
  {
    const IntervalIteratorContext *VKL_INTEROP_UNIFORM context;
  };
#endif

#ifndef __ISPC_STRUCT_HitIteratorShared__
#define __ISPC_STRUCT_HitIteratorShared__
  // must be first member of hit iterator structs
  struct HitIteratorShared
  {
    const HitIteratorContext *VKL_INTEROP_UNIFORM context;
  };
#endif

#ifdef __cplusplus
}
#endif  // __cplusplus
