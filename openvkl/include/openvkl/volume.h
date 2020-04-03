// Copyright 2019 Intel Corporation
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
struct Volume : public ManagedObject
{
};
#else
typedef ManagedObject Volume;
#endif

typedef Volume *VKLVolume;

// cell types definition for unstructured volumes, values are set to match VTK
typedef enum
# if __cplusplus >= 201103L
: uint8_t
#endif
{
  VKL_TETRAHEDRON = 10,
  VKL_HEXAHEDRON = 12,
  VKL_WEDGE = 13,
  VKL_PYRAMID = 14
} VKLUnstructuredCellType;

// AMR volume interpolation methods
typedef enum
# if __cplusplus >= 201103L
: uint8_t
#endif
{
  VKL_AMR_CURRENT,
  VKL_AMR_FINEST,
  VKL_AMR_OCTANT
} VKLAMRMethod;

#ifdef __cplusplus
extern "C" {
#endif

OPENVKL_INTERFACE
VKLVolume vklNewVolume(const char *type);

OPENVKL_INTERFACE
float vklComputeSample(VKLVolume volume, const vkl_vec3f *objectCoordinates);

OPENVKL_INTERFACE
void vklComputeSample4(const int *valid,
                       VKLVolume volume,
                       const vkl_vvec3f4 *objectCoordinates,
                       float *samples);

OPENVKL_INTERFACE
void vklComputeSample8(const int *valid,
                       VKLVolume volume,
                       const vkl_vvec3f8 *objectCoordinates,
                       float *samples);

OPENVKL_INTERFACE
void vklComputeSample16(const int *valid,
                        VKLVolume volume,
                        const vkl_vvec3f16 *objectCoordinates,
                        float *samples);

OPENVKL_INTERFACE
vkl_vec3f vklComputeGradient(VKLVolume volume,
                             const vkl_vec3f *objectCoordinates);

OPENVKL_INTERFACE
void vklComputeGradient4(const int *valid,
                         VKLVolume volume,
                         const vkl_vvec3f4 *objectCoordinates,
                         vkl_vvec3f4 *gradients);

OPENVKL_INTERFACE
void vklComputeGradient8(const int *valid,
                         VKLVolume volume,
                         const vkl_vvec3f8 *objectCoordinates,
                         vkl_vvec3f8 *gradients);

OPENVKL_INTERFACE
void vklComputeGradient16(const int *valid,
                          VKLVolume volume,
                          const vkl_vvec3f16 *objectCoordinates,
                          vkl_vvec3f16 *gradients);

OPENVKL_INTERFACE
vkl_box3f vklGetBoundingBox(VKLVolume volume);

OPENVKL_INTERFACE vkl_range1f vklGetValueRange(VKLVolume volume);

#ifdef __cplusplus
}  // extern "C"
#endif
