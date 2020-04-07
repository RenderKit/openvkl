// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Error codes returned by various API and callback functions
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  VKL_NO_ERROR         = 0,  // No error has been recorded
  VKL_UNKNOWN_ERROR    = 1,  // An unknown error has occured
  VKL_INVALID_ARGUMENT = 2,  // An invalid argument is specified
  VKL_INVALID_OPERATION =
      3,  // The operation is not allowed for the specified object
  VKL_OUT_OF_MEMORY =
      4,  // There is not enough memory left to execute the command
  VKL_UNSUPPORTED_CPU =
      5,  // The CPU is not supported as it does not support SSE4.1
} VKLError;
