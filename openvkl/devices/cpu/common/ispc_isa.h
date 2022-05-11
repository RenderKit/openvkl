// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// this header is shared with ISPC

#ifdef __cplusplus
#include <cstdint>
#include <string>
#endif

#ifdef __cplusplus
typedef enum : uint32_t
#else
typedef enum
#endif
{
  VKL_ISPC_TARGET_NEON,
  VKL_ISPC_TARGET_SSE2,
  VKL_ISPC_TARGET_SSE4,
  VKL_ISPC_TARGET_AVX,
  VKL_ISPC_TARGET_AVX2,
  VKL_ISPC_TARGET_AVX512KNL,
  VKL_ISPC_TARGET_AVX512SKX,

  // Guard value.
  VKL_ISPC_TARGET_UNKNOWN = 9999999
} VKLISPCTarget;

#ifdef __cplusplus
inline std::string stringForVKLISPCTarget(VKLISPCTarget target)
{
  switch (target) {
  case VKL_ISPC_TARGET_NEON:
    return "NEON";
  case VKL_ISPC_TARGET_SSE2:
    return "SSE2";
  case VKL_ISPC_TARGET_SSE4:
    return "SSE4";
  case VKL_ISPC_TARGET_AVX:
    return "AVX";
  case VKL_ISPC_TARGET_AVX2:
    return "AVX2";
  case VKL_ISPC_TARGET_AVX512KNL:
    return "AVX512KNL";
  case VKL_ISPC_TARGET_AVX512SKX:
    return "AVX512SKX";
  case VKL_ISPC_TARGET_UNKNOWN:
  default:
    return "UNKNOWN";
  }
}
#endif
