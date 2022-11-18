// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <cstdlib>
#else
#include <stdint.h>
#include <stdlib.h>
#endif

#include <openvkl/common.h>

typedef APIObject VKLSampler;

#ifdef __cplusplus
extern "C" {
#endif

OPENVKL_INTERFACE
void vklInit();

OPENVKL_INTERFACE
float vklComputeSample(const VKLSampler *sampler,
                       const vkl_vec3f *objectCoordinates,
                       unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                       float time VKL_DEFAULT_VAL(= 0));

OPENVKL_INTERFACE
void vklComputeSampleM(const VKLSampler *sampler,
                       const vkl_vec3f *objectCoordinates,
                       float *samples,
                       unsigned int M,
                       const unsigned int *attributeIndices,
                       float time VKL_DEFAULT_VAL(= 0));

OPENVKL_INTERFACE
vkl_vec3f vklComputeGradient(const VKLSampler *sampler,
                             const vkl_vec3f *objectCoordinates,
                             unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                             float time VKL_DEFAULT_VAL(= 0));

#ifdef __cplusplus
}  // extern "C"
#endif
