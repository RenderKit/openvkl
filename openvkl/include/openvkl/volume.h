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
#include "device.h"

#ifdef __cplusplus
struct VKLVolume : public VKLObject
{
};
#else
typedef VKLObject VKLVolume;
#endif

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

NOWARN_C_LINKAGE_PUSH
OPENVKL_INTERFACE
VKLVolume vklNewVolume(VKLDevice device, const char *type);
NOWARN_C_LINKAGE_POP

OPENVKL_INTERFACE
vkl_box3f vklGetBoundingBox(VKLVolume volume);

OPENVKL_INTERFACE unsigned int vklGetNumAttributes(VKLVolume volume);

OPENVKL_INTERFACE vkl_range1f vklGetValueRange(
    VKLVolume volume, unsigned int attributeIndex VKL_DEFAULT_VAL(= 0));

// The below are primarily used to enable ISPC bindings, which cannot handle
// returning structs by value.

OPENVKL_INTERFACE void vklGetBoundingBoxRef(const VKLVolume *volume,
                                            vkl_box3f *boundingBox);

OPENVKL_INTERFACE unsigned int vklGetNumAttributesRef(const VKLVolume *volume);

OPENVKL_INTERFACE void vklGetValueRangeRef(const VKLVolume *volume,
                                           unsigned int attributeIndex,
                                           vkl_range1f *valueRange);

#ifdef __cplusplus
}  // extern "C"
#endif
