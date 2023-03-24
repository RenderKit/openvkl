// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace ispc {

  struct DualCellID
  {
    // position of LOWER-LEFT COORDINATE (ie, CENTER of lower-left cell
    vec3f pos;
    // width of dual cell, also doubles as level indicator
    float width;
  };

  struct DualCell
  {
    // input parameters that specify the cell. coords must be the actual cell
    // centers on the desired level
    DualCellID cellID;

    // interpolation weights
    vec3f weights;

    // values as found the by the query
    float value[8];
    float actualWidth[8];
    bool isLeaf[8];
  };

  inline vec3f dualCellLerpWeightsForLevel(const vec3f &P,
                                           const float cellWidth)
  {
    const float halfCellWidth = 0.5f * cellWidth;
    const float rcpCellWidth  = rcp(cellWidth);
    const vec3f xfmed         = (P - halfCellWidth) * rcpCellWidth;
    const vec3f f_idx         = floor(xfmed);
    return (xfmed - f_idx);
  }

  inline void initDualCell(DualCell &D, const vec3f &P, const AMRLevel &level)
  {
    const float cellWidth     = level.cellWidth;
    const float halfCellWidth = 0.5f * cellWidth;
    const float rcpCellWidth  = rcp(cellWidth);
    const vec3f xfmed         = (P - halfCellWidth) * rcpCellWidth;
    const vec3f f_idx         = floor(xfmed);
    D.cellID.pos              = f_idx * cellWidth + halfCellWidth;
    D.cellID.width            = cellWidth;
    D.weights                 = xfmed - f_idx;
  }

  inline void initDualCell(DualCell &D, const vec3f &P, const float cellWidth)
  {
    const float halfCellWidth = cellWidth * 0.5f;
    const float rcpCellWidth  = rcp(cellWidth);
    const vec3f xfmed         = (P - halfCellWidth) * rcpCellWidth;
    const vec3f f_idx         = floor(xfmed);
    D.cellID.pos              = f_idx * cellWidth + halfCellWidth;
    D.cellID.width            = cellWidth;
    D.weights                 = xfmed - f_idx;
  }

  inline bool allCornersAreLeaves(const DualCell &D)
  {
    return D.isLeaf[0] & D.isLeaf[1] & D.isLeaf[2] & D.isLeaf[3] & D.isLeaf[4] &
           D.isLeaf[5] & D.isLeaf[6] & D.isLeaf[7];
  }

  inline bool allCornersArePresent(const DualCell &D)
  {
    return (D.actualWidth[0] == D.cellID.width) &
           (D.actualWidth[1] == D.cellID.width) &
           (D.actualWidth[2] == D.cellID.width) &
           (D.actualWidth[3] == D.cellID.width) &
           (D.actualWidth[4] == D.cellID.width) &
           (D.actualWidth[5] == D.cellID.width) &
           (D.actualWidth[6] == D.cellID.width) &
           (D.actualWidth[7] == D.cellID.width);
  }

  inline float lerp(const DualCell &D)
  {
    const vec3f &w   = D.weights;
    const float f000 = D.value[C000];
    const float f001 = D.value[C001];
    const float f010 = D.value[C010];
    const float f011 = D.value[C011];
    const float f100 = D.value[C100];
    const float f101 = D.value[C101];
    const float f110 = D.value[C110];
    const float f111 = D.value[C111];

    const float f00 = (1.f - w.x) * f000 + w.x * f001;
    const float f01 = (1.f - w.x) * f010 + w.x * f011;
    const float f10 = (1.f - w.x) * f100 + w.x * f101;
    const float f11 = (1.f - w.x) * f110 + w.x * f111;

    const float f0 = (1.f - w.y) * f00 + w.y * f01;
    const float f1 = (1.f - w.y) * f10 + w.y * f11;

    const float f = (1.f - w.z) * f0 + w.z * f1;
    return f;
  }

  inline float lerpWithExplicitWeights(const DualCell &D, const vec3f &w)
  {
    const float f000 = D.value[C000];
    const float f001 = D.value[C001];
    const float f010 = D.value[C010];
    const float f011 = D.value[C011];
    const float f100 = D.value[C100];
    const float f101 = D.value[C101];
    const float f110 = D.value[C110];
    const float f111 = D.value[C111];

    const float f00 = (1.f - w.x) * f000 + w.x * f001;
    const float f01 = (1.f - w.x) * f010 + w.x * f011;
    const float f10 = (1.f - w.x) * f100 + w.x * f101;
    const float f11 = (1.f - w.x) * f110 + w.x * f111;

    const float f0 = (1.f - w.y) * f00 + w.y * f01;
    const float f1 = (1.f - w.y) * f10 + w.y * f11;

    const float f = (1.f - w.z) * f0 + w.z * f1;
    return f;
  }

  inline float lerpAlpha(const DualCell &D)
  {
    const vec3f &w   = D.weights;
    const float f000 = D.actualWidth[C000] == D.cellID.width ? 1.f : 0.f;
    const float f001 = D.actualWidth[C001] == D.cellID.width ? 1.f : 0.f;
    const float f010 = D.actualWidth[C010] == D.cellID.width ? 1.f : 0.f;
    const float f011 = D.actualWidth[C011] == D.cellID.width ? 1.f : 0.f;
    const float f100 = D.actualWidth[C100] == D.cellID.width ? 1.f : 0.f;
    const float f101 = D.actualWidth[C101] == D.cellID.width ? 1.f : 0.f;
    const float f110 = D.actualWidth[C110] == D.cellID.width ? 1.f : 0.f;
    const float f111 = D.actualWidth[C111] == D.cellID.width ? 1.f : 0.f;

    const float f00 = (1.f - w.x) * f000 + w.x * f001;
    const float f01 = (1.f - w.x) * f010 + w.x * f011;
    const float f10 = (1.f - w.x) * f100 + w.x * f101;
    const float f11 = (1.f - w.x) * f110 + w.x * f111;

    const float f0 = (1.f - w.y) * f00 + w.y * f01;
    const float f1 = (1.f - w.y) * f10 + w.y * f11;

    const float f = (1.f - w.z) * f0 + w.z * f1;
    return f;
  }

  struct FindEightStack
  {
    bool act_lo[3];
    bool act_hi[3];
    int32 nodeID;
  };

  void findDualCell(const AMR *self, DualCell &dual)
  {
    const vec3f _P0 =
        clamp(dual.cellID.pos, make_vec3f(0.f), self->maxValidPos);
    const vec3f _P1 = clamp(dual.cellID.pos + dual.cellID.width,
                            make_vec3f(0.f),
                            self->maxValidPos);

    const float *const p0 = &_P0.x;
    const float *const p1 = &_P1.x;

    const float *const lo = p0;
    const float *const hi = p1;

#define STACK_SIZE 64
    FindEightStack stack[STACK_SIZE];
    FindEightStack *stackPtr = &stack[0];

    int32 leafList[8];
    int32 numLeaves = 0;

    bool act_lo[3] = {true, true, true};
    bool act_hi[3] = {true, true, true};
    int nodeID     = 0;
    while (true) {
      const KDTreeNode &node = self->node[nodeID];
      const uint32 childID   = getOfs(node);
      if (isLeaf(node)) {
        assert(numLeaves < (8));
        leafList[numLeaves++] = childID;
        // go on to popping ...
      } else {
        const int dim        = getDim(node);
        const float pos      = getPos(node);
        const bool in_active = (act_lo[0] | act_hi[0]) &
                               (act_lo[1] | act_hi[1]) &
                               (act_lo[2] | act_hi[2]);
        const bool go_left = ((act_lo[dim] & (lo[dim] < pos)) |
                              (act_hi[dim] & (hi[dim] < pos))) &
                             in_active;
        const bool go_right = ((act_lo[dim] & (lo[dim] >= pos)) |
                               (act_hi[dim] & (hi[dim] >= pos))) &
                              in_active;

        if (!go_right) {
          // all to the left: go left and iterate
          nodeID = childID + 0;
          continue;
        }

        if (!go_left) {
          // all to the left: go left and iterate
          nodeID = childID + 1;
          continue;
        }

        // push right
        stackPtr->nodeID = childID + 1;
        for (int i = 0; i < 3; i++) {
          stackPtr->act_lo[i] = act_lo[i];
          stackPtr->act_hi[i] = act_hi[i];
        }
        stackPtr->act_lo[dim] &= (lo[dim] >= pos);
        stackPtr->act_hi[dim] &= (hi[dim] >= pos);
        ++stackPtr;
        assert(stackPtr - stack < STACK_SIZE);

        // go left
        nodeID = childID + 0;
        act_lo[dim] &= (lo[dim] < pos);
        act_hi[dim] &= (hi[dim] < pos);
        continue;
      }
      // pop:
      if (stackPtr == stack)
        break;
      --stackPtr;
      for (int i = 0; i < 3; i++) {
        act_lo[i] = stackPtr->act_lo[i];
        act_hi[i] = stackPtr->act_hi[i];
      }
      nodeID = stackPtr->nodeID;
    }

    // now, process leaves we found
    const float desired_width = dual.cellID.width;
    {
      for (int leafID = 0; leafID < numLeaves; leafID++) {
        const AMRLeaf *leaf = &self->leaf[leafList[leafID]];
        const bool valid_x0 =
            _P0.x >= leaf->bounds.lower.x & _P0.x < leaf->bounds.upper.x;
        const bool valid_y0 =
            _P0.y >= leaf->bounds.lower.y & _P0.y < leaf->bounds.upper.y;
        const bool valid_z0 =
            _P0.z >= leaf->bounds.lower.z & _P0.z < leaf->bounds.upper.z;

        const bool valid_x1 =
            _P1.x >= leaf->bounds.lower.x & _P1.x < leaf->bounds.upper.x;
        const bool valid_y1 =
            _P1.y >= leaf->bounds.lower.y & _P1.y < leaf->bounds.upper.y;
        const bool valid_z1 =
            _P1.z >= leaf->bounds.lower.z & _P1.z < leaf->bounds.upper.z;

        const bool anyValid = (valid_x0 | valid_x1) & (valid_y0 | valid_y1) &
                              (valid_z0 | valid_z1);
        if (!(anyValid))
          continue;

        int brickID           = 0;
        bool isLeaf           = true;
        const AMRBrick *brick = leaf->brickList[brickID];
        while (brick->cellWidth < desired_width) {
          brick  = leaf->brickList[++brickID];
          isLeaf = false;
        }

        const Data1D v  = brick->value;
        const vec3f rp0 = (_P0 - brick->bounds.lower) * brick->bounds_scale;
        const vec3f rp1 = (_P1 - brick->bounds.lower) * brick->bounds_scale;

        const vec3f f_bc0 = floor(rp0 * brick->f_dims);
        const vec3f f_bc1 = floor(rp1 * brick->f_dims);

        // index offsets to neighbor cells
        const float f_idx_dx0 = f_bc0.x;
        const float f_idx_dy0 = f_bc0.y * brick->f_dims.x;
        const float f_idx_dz0 = f_bc0.z * brick->f_dims.x * brick->f_dims.y;

        const float f_idx_dx1 = f_bc1.x;
        const float f_idx_dy1 = f_bc1.y * brick->f_dims.x;
        const float f_idx_dz1 = f_bc1.z * brick->f_dims.x * brick->f_dims.y;

#define DOCORNER(X, Y, Z)                                                    \
  if (valid_z##Z & valid_y##Y & valid_x##X) {                                \
    const float fidx              = f_idx_dx##X + f_idx_dy##Y + f_idx_dz##Z; \
    dual.value[Z * 4 + Y * 2 + X] = get_float(v, uint32(fidx));              \
    dual.actualWidth[Z * 4 + Y * 2 + X] = brick->cellWidth;                  \
    dual.isLeaf[Z * 4 + Y * 2 + X]      = isLeaf;                            \
  }
        DOCORNER(0, 0, 0);
        DOCORNER(0, 0, 1);
        DOCORNER(0, 1, 0);
        DOCORNER(0, 1, 1);
        DOCORNER(1, 0, 0);
        DOCORNER(1, 0, 1);
        DOCORNER(1, 1, 0);
        DOCORNER(1, 1, 1);
#undef DOCORNER
      }
    }
  }

}  // namespace ispc
