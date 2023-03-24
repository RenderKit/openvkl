// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../../cpu/volume/amr/KDTreeShared.h"

namespace ispc {

  inline uint32 getDim(const KDTreeNode &node)
  {
    return node.dim_and_ofs >> 30;
  }

  inline uint32 getOfs(const KDTreeNode &node)
  {
    return node.dim_and_ofs & ((1 << 30) - 1);
  }

  inline bool isLeaf(const KDTreeNode &node)
  {
    return getDim(node) == 3;
  }

  inline float getPos(const KDTreeNode &node)
  {
    return floatbits(node.pos_or_numItems);
  }

  inline float getNumItems(const KDTreeNode &node)
  {
    return node.pos_or_numItems;
  }

}  // namespace ispc
