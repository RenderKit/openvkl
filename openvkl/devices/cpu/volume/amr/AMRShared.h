// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "../common/DataShared.h"
#include "openvkl/VKLDataType.h"
// ours
#include "KDTreeShared.h"

/*! enum to symbolically iterate the 8 corners of an octant */
enum
{
  C000 = 0,
  C001,
  C010,
  C011,
  C100,
  C101,
  C110,
  C111
};

struct AMRBrick
{
  /*! bounding box of integer coordinates of cells. note that
    this EXCLUDES the width of the rightmost cell: ie, a 4^3
    box at root level pos (0,0,0) would have a _box_ of
    [(0,0,0)-(3,3,3)] (because 3,3,3 is the highest valid
    coordinate in this box!), while its bounds would be
    [(0,0,0)-(4,4,4)]. Make sure to NOT use box.size() for the
    grid dimensions, since this will always be one lower than
    the dims of the grid.... */
  box3i box;
  //! level this brick is at
  int level;
  // width of each cell in this level
  float cellWidth;

  /* world bounds, including entire cells. ie, a 4^3 root brick
     at (0,0,0) would have bounds [(0,0,0)-(4,4,4)] (as opposed
     to the 'box' value, see above!) */
  box3f bounds;
// pointer to the actual data values stored in this brick
#ifdef __cplusplus
  ispc::Data1D *value;
#else
  Data1D *value;
#endif
  // dimensions of this box's data
  vec3i dims;
  // scale factor from grid space to world space (ie,1.f/cellWidth)
  float gridToWorldScale;

  // rcp(bounds.upper-bounds.lower);
  vec3f bounds_scale;
  // dimensions, in float
  vec3f f_dims;
};

struct AMRLeaf
{
  AMRBrick **brickList;
  box3f bounds;
  range1f valueRange;
};

struct AMRLevel
{
  float cellWidth;
  /* do not use these values: they are computed on the C side, at
     apparently different accuracy than when they get computed on the
     ISPC side - this gets "sometimes" inconsistent results */
  float doNotUse_rcpCellWidth;
  float doNotUse_halfCellWidth;
  int level;
};

struct AMR
{
  /*! "item list" array - each leaf node in the tree points into this
    array, and the 'num' elements following the pointed-to-location
    are the bricks stored at this leaf */
  AMRLeaf *leaf;
  // AMRBrick *uniform *uniform item;
  KDTreeNode *node;
  AMRLevel *level;
  AMRLevel *finestLevel;
  vkl_uint32 numNodes;
  vkl_uint32 numLeaves;
  vkl_uint32 numLevels;
  float finestLevelCellWidth;

  box3f worldBounds;
  vec3f maxValidPos;

  //! Voxel type.
  VKL_INTEROP_UNIFORM VKLDataType voxelType;

  //! Voxel data accessor.
  float (*VKL_INTEROP_UNIFORM getVoxel)(
#ifdef __cplusplus
      ispc::Data1D *VKL_INTEROP_UNIFORM data,
#else
      Data1D *VKL_INTEROP_UNIFORM data,
#endif
      const VKL_INTEROP_VARYING vkl_uint32 index);
};
