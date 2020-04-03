// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Log levels which can be set on a driver via "logLevel" parameter or
// "OPENVKL_LOG_LEVEL" environment variable
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  VKL_LOG_DEBUG   = 1,
  VKL_LOG_INFO    = 2,
  VKL_LOG_WARNING = 3,
  VKL_LOG_ERROR   = 4,
  VKL_LOG_NONE    = 5,
} VKLLogLevel;
