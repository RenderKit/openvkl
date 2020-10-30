// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <cstdlib>
#else
#include <stdint.h>
#include <stdlib.h>
#endif

#include "common.h"

#ifdef __cplusplus
struct Sampler : public ManagedObject
{
};
#else
typedef ManagedObject Sampler;
#endif

typedef Sampler *VKLSampler;

#ifdef __cplusplus
extern "C" {
#endif

OPENVKL_INTERFACE VKLSampler vklNewSampler(VKLVolume volume);

// single attribute sampling

OPENVKL_INTERFACE
float vklComputeSample(VKLSampler sampler,
                       const vkl_vec3f *objectCoordinates,
                       unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                       float time VKL_DEFAULT_VAL(= 0));

OPENVKL_INTERFACE
void vklComputeSample4(const int *valid,
                       VKLSampler sampler,
                       const vkl_vvec3f4 *objectCoordinates,
                       float *samples,
                       unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                       const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSample8(const int *valid,
                       VKLSampler sampler,
                       const vkl_vvec3f8 *objectCoordinates,
                       float *samples,
                       unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                       const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSample16(const int *valid,
                        VKLSampler sampler,
                        const vkl_vvec3f16 *objectCoordinates,
                        float *samples,
                        unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                        const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSampleN(VKLSampler sampler,
                       unsigned int N,
                       const vkl_vec3f *objectCoordinates,
                       float *samples,
                       unsigned int attributeIndex VKL_DEFAULT_VAL(= 0),
                       const float *times VKL_DEFAULT_VAL(= nullptr));

// multi-attribute sampling

OPENVKL_INTERFACE
void vklComputeSampleM(VKLSampler sampler,
                       const vkl_vec3f *objectCoordinates,
                       float *samples,
                       unsigned int M,
                       const unsigned int *attributeIndices,
                       const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSampleM4(const int *valid,
                        VKLSampler sampler,
                        const vkl_vvec3f4 *objectCoordinates,
                        float *samples,
                        unsigned int M,
                        const unsigned int *attributeIndices,
                        const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSampleM8(const int *valid,
                        VKLSampler sampler,
                        const vkl_vvec3f8 *objectCoordinates,
                        float *samples,
                        unsigned int M,
                        const unsigned int *attributeIndices,
                        const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSampleM16(const int *valid,
                         VKLSampler sampler,
                         const vkl_vvec3f16 *objectCoordinates,
                         float *samples,
                         unsigned int M,
                         const unsigned int *attributeIndices,
                         const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
void vklComputeSampleMN(VKLSampler sampler,
                        unsigned int N,
                        const vkl_vec3f *objectCoordinates,
                        float *samples,
                        unsigned int M,
                        const unsigned int *attributeIndices,
                        const float *times VKL_DEFAULT_VAL(= nullptr));

OPENVKL_INTERFACE
vkl_vec3f vklComputeGradient(VKLSampler sampler,
                             const vkl_vec3f *objectCoordinates,
                             unsigned int attributeIndex VKL_DEFAULT_VAL(= 0));

OPENVKL_INTERFACE
void vklComputeGradient4(const int *valid,
                         VKLSampler sampler,
                         const vkl_vvec3f4 *objectCoordinates,
                         vkl_vvec3f4 *gradients,
                         unsigned int attributeIndex VKL_DEFAULT_VAL(= 0));

OPENVKL_INTERFACE
void vklComputeGradient8(const int *valid,
                         VKLSampler sampler,
                         const vkl_vvec3f8 *objectCoordinates,
                         vkl_vvec3f8 *gradients,
                         unsigned int attributeIndex VKL_DEFAULT_VAL(= 0));

OPENVKL_INTERFACE
void vklComputeGradient16(const int *valid,
                          VKLSampler sampler,
                          const vkl_vvec3f16 *objectCoordinates,
                          vkl_vvec3f16 *gradients,
                          unsigned int attributeIndex VKL_DEFAULT_VAL(= 0));

OPENVKL_INTERFACE
void vklComputeGradientN(VKLSampler sampler,
                         unsigned int N,
                         const vkl_vec3f *objectCoordinates,
                         vkl_vec3f *gradients,
                         unsigned int attributeIndex VKL_DEFAULT_VAL(= 0));

#ifdef __cplusplus
}  // extern "C"
#endif
