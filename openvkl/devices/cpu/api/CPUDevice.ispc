// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "../common/ispc_isa.h"

export uniform int32 EXPORT_UNIQUE(ISPC_getProgramCount)
{
  return programCount;
}

// see
// https://ispc.github.io/faq.html#how-can-i-determine-at-run-time-which-vector-instruction-set-s-instructions-were-selected-to-execute
export uniform uint32 EXPORT_UNIQUE(ISPC_getTarget)
{
#if defined(ISPC_TARGET_NEON)
  return VKL_ISPC_TARGET_NEON;
#elif defined(ISPC_TARGET_SSE2)
  return VKL_ISPC_TARGET_SSE2;
#elif defined(ISPC_TARGET_SSE4)
  return VKL_ISPC_TARGET_SSE4;
#elif defined(ISPC_TARGET_AVX)
  return VKL_ISPC_TARGET_AVX;
#elif defined(ISPC_TARGET_AVX2)
  return VKL_ISPC_TARGET_AVX2;
#elif defined(ISPC_TARGET_AVX512KNL)
  return VKL_ISPC_TARGET_AVX512KNL;
#elif defined(ISPC_TARGET_AVX512SKX)
  return VKL_ISPC_TARGET_AVX512SKX;
#else
  return VKL_ISPC_TARGET_UNKNOWN;
#endif
}
