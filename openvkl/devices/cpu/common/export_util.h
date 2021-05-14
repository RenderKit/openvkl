// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifndef VKL_TARGET_WIDTH
#error "export_util.h included without VKL_TARGET_WIDTH defined"
#endif

#define CONCAT2(A, B) A##B
#define CONCAT1(A, B) CONCAT2(A, B)

#define CONCAT_ARGS_V(A, ...) A(__VA_ARGS__)

#define EXPORT_UNIQUE(name, ...) \
  CONCAT_ARGS_V(CONCAT1(name, VKL_TARGET_WIDTH), __VA_ARGS__)

#if defined(ISPC)
#define CALL_ISPC(function, ...) \
  CONCAT_ARGS_V(CONCAT1(function, VKL_TARGET_WIDTH), __VA_ARGS__)
#else
#define CALL_ISPC(function, ...) \
  CONCAT_ARGS_V(CONCAT1(ispc::function, VKL_TARGET_WIDTH), __VA_ARGS__)
#endif
