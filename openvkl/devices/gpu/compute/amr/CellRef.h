// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "AMR.h"
#include "FindStack.h"
#include "KDTree.h"

namespace ispc {

  // a reference to a given cell on a given level; this is what a 'node
  // location' kernel will return
  struct CellRef
  {
    // lower left front position, in unit grid space
    vec3f pos;

    // width of cell, also doubles as level indicator
    float width;

    // value at this cell
    float value;
  };

  inline vec3f centerOf(const CellRef &cr)
  {
    return cr.pos + make_vec3f(0.5f * cr.width);
  }

  inline void set(CellRef &cr,
                  const vec3f &pos,
                  const float width,
                  const float value)
  {
    cr.pos   = pos;
    cr.width = width;
    cr.value = value;
  }

  inline CellRef findLeafCell(const AMR *self, const vec3f &_worldSpacePos)
  {
    const vec3f worldSpacePos =
        max(make_vec3f(0.f), min(self->worldBounds.upper, _worldSpacePos));
    const float *const samplePos = &worldSpacePos.x;

    FindStack stack[16];
    FindStack *stackPtr = pushStack(&stack[0], 0);

    while (stackPtr > stack) {
      --stackPtr;
      if (stackPtr->active) {
        const uint32 nodeID   = stackPtr->nodeID;
        const KDTreeNode node = self->node[nodeID];
        if (isLeaf(node)) {
          const AMRLeaf *leaf   = &self->leaf[getOfs(node)];
          const AMRBrick *brick = leaf->brickList[0];
          const vec3f relBrickPos =
              (worldSpacePos - brick->bounds.lower) * brick->bounds_scale;
          // brick coords: integer cell coordinates inside brick
          // OPT: the same calculations as below, just in
          // floats. this works as long as all values we calculate
          // with are fraction-less values (so essentially ints) and
          // fit into 24 bits mantissa (which they easily should for
          // any brick
          const vec3f f_bc = floor(relBrickPos * brick->f_dims);
          CellRef ret;
          const uint32 idx =
              (int)(f_bc.x +
                    brick->f_dims.x * (f_bc.y + brick->f_dims.y * (f_bc.z)));
          ret.pos   = brick->bounds.lower + f_bc * brick->cellWidth;
          ret.value = get_float(brick->value, idx);
          ret.width = brick->cellWidth;
          return ret;
        } else {
          const uint32 childID = getOfs(node);
          if (samplePos[getDim(node)] >= getPos(node)) {
            stackPtr = pushStack(stackPtr, childID + 1);
          } else {
            stackPtr = pushStack(stackPtr, childID);
          }
        }
      }
    }

    // should never reach this point
    CellRef ret;
    ret.pos   = make_vec3f(0.f);
    ret.width = 0.f;
    ret.value = 0.f;
    return ret;
  }

}  // namespace ispc
