// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "UnstructuredVolumeBaseShared.h"

#ifdef __cplusplus
namespace ispc {
#endif  // __cplusplus

  typedef enum
  {
    VKL_TETRAHEDRON = 10,
    VKL_HEXAHEDRON  = 12,
    VKL_WEDGE       = 13,
    VKL_PYRAMID     = 14
  } CellType;

  struct LeafNode
  {
    VKL_INTEROP_UNIFORM Node super;
    VKL_INTEROP_UNIFORM box3fa bounds;
  };

  struct LeafNodeSingle
  {
    VKL_INTEROP_UNIFORM LeafNode super;
    VKL_INTEROP_UNIFORM vkl_uint64 cellID;
  };

  struct LeafNodeMulti
  {
    VKL_INTEROP_UNIFORM LeafNode super;
    VKL_INTEROP_UNIFORM vkl_uint64 numCells;
    VKL_INTEROP_UNIFORM vkl_uint64 *VKL_INTEROP_UNIFORM cellIDs;
  };

  struct InnerNode
  {
    VKL_INTEROP_UNIFORM Node super;
    VKL_INTEROP_UNIFORM box3fa bounds[2];
    VKL_INTEROP_UNIFORM Node *VKL_INTEROP_UNIFORM children[2];
  };

  struct VKLUnstructuredVolume
  {
    VKLUnstructuredBase super;

    // vertex data
    VKL_INTEROP_UNIFORM Data1D vertex;
    VKL_INTEROP_UNIFORM Data1D vertexValue;  // attribute value at each vertex

    // index data
    VKL_INTEROP_UNIFORM Data1D index;  // indices into vertices array
    VKL_INTEROP_UNIFORM bool
        index32Bit;  // true if index is 32-bit integer, false if 64-bit

    // cell data
    VKL_INTEROP_UNIFORM Data1D cell;  // cell offsets in indices array
    VKL_INTEROP_UNIFORM bool
        cell32Bit;  // true if cell offset is 32-bit integer, false if 64-bit
    VKL_INTEROP_UNIFORM uint32 cellSkipIds;  // skip indices when index array
                                             // contain other data e.g. size
    VKL_INTEROP_UNIFORM Data1D cellType;   // cell type array
    VKL_INTEROP_UNIFORM Data1D cellValue;  // attribute value at each cell

    const vec3f *VKL_INTEROP_UNIFORM faceNormals;
    const float *VKL_INTEROP_UNIFORM iterativeTolerance;

    VKL_INTEROP_UNIFORM vec3f gradientStep;

    VKL_INTEROP_UNIFORM bool hexIterative;
  };

#ifdef __cplusplus
}
#endif  // __cplusplus
