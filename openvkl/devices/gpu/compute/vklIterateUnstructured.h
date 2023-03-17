// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common.h"
#include "iterator_common.h"

#include "../../cpu/iterator/IteratorContextShared.h"
#include "../../cpu/iterator/IteratorShared.h"

namespace ispc {

  ////////////////////////////////////////////////////////////////////////////
  // struct definitions //////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  struct UnstructuredTraversalState
  {
    Node *node;
    uint32_t bitstack;
  };

  struct UnstructuredIntervalIterator
  {
    // DefaultHitIteratorIntervalIterator super;
    IntervalIteratorShared super;
    bool elementaryCellIterationSupported;

    const SamplerShared *sampler;
    vec3f origin;
    vec3f direction;
    box1f tRange;

    UnstructuredTraversalState traversalState;
  };

  ////////////////////////////////////////////////////////////////////////////
  // cell intersections //////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  inline vec3f stable_tri_normal(const vec3f &a, const vec3f &b, const vec3f &c)
  {
    const float ab_x = a.z * b.y;
    const float ab_y = a.x * b.z;
    const float ab_z = a.y * b.x;
    const float bc_x = b.z * c.y;
    const float bc_y = b.x * c.z;
    const float bc_z = b.y * c.x;
    const vec3f cross_ab =
        vec3f(a.y * b.z - ab_x, a.z * b.x - ab_y, a.x * b.y - ab_z);
    const vec3f cross_bc =
        vec3f(b.y * c.z - bc_x, b.z * c.x - bc_y, b.x * c.y - bc_z);
    const bool sx = absf(ab_x) < absf(bc_x);
    const bool sy = absf(ab_y) < absf(bc_y);
    const bool sz = absf(ab_z) < absf(bc_z);
    return vec3f(sx ? cross_ab.x : cross_bc.x,
                 sy ? cross_ab.y : cross_bc.y,
                 sz ? cross_ab.z : cross_bc.z);
  }

  inline void intersectRayTri(const vec3f &origin,
                              const vec3f &direction,
                              const vec3f &p0,
                              const vec3f &p1,
                              const vec3f &p2,
                              bool &foundEntrance,
                              bool &foundExit,
                              box1f &intersectedTRange)
  {
    const vec3f v0 = p0 - origin;
    const vec3f v1 = p1 - origin;
    const vec3f v2 = p2 - origin;

    // calculate triangle edges
    const vec3f e0 = v2 - v0;
    const vec3f e1 = v0 - v1;
    const vec3f e2 = v1 - v2;

    // perform edge tests
    const float U = dot(cross(e0, v2 + v0), direction);
    const float V = dot(cross(e1, v0 + v1), direction);
    const float W = dot(cross(e2, v1 + v2), direction);
    if (!foundEntrance) {
      const float UVW = U + V + W;
      const float eps = 1e-6f * absf(UVW);
      bool isenterhit = (min(U, min(V, W)) >= -eps);
      if (isenterhit) {
        const vec3f Ng  = stable_tri_normal(e0, e1, e2);
        const float den = 2.f * dot(Ng, direction);

        // perform depth test
        if (den != 0.f) {
          const float T           = 2.f * dot(v0, Ng);
          intersectedTRange.lower = rcp(den) * T;
          foundEntrance           = true;
          return;
        }
      }
    }
    if (!foundExit) {
      const float UVW = U + V + W;
      const float eps = 1e-6f * absf(UVW);
      bool isexithit  = (max(U, max(V, W)) <= eps);
      if (isexithit) {
        const vec3f Ng  = stable_tri_normal(e0, e1, e2);
        const float den = 2.f * dot(Ng, direction);

        // perform depth test
        if (den != 0.f) {
          const float T           = 2.f * dot(v0, Ng);
          intersectedTRange.upper = rcp(den) * T;
          foundExit               = true;
          return;
        }
      }
    }
    return;
  }

  inline box1f intersectRayTet(const vec3f &origin,
                               const vec3f &direction,
                               const box1f &rangeLimit,
                               const VKLUnstructuredVolume *volume,
                               const uint64 cellID)
  {
    // Get cell offset in index buffer
    const uint64 cOffset = getCellOffset(volume, cellID);

    const uint64 planes[4][3] = {{cOffset + 2, cOffset + 0, cOffset + 1},
                                 {cOffset + 3, cOffset + 1, cOffset + 0},
                                 {cOffset + 3, cOffset + 2, cOffset + 1},
                                 {cOffset + 2, cOffset + 3, cOffset + 0}};
    box1f intersectedTRange   = box1f(neg_inf, inf);
    bool foundExit            = false;
    bool foundEntrance        = false;
    for (int i = 0; i < 4; ++i) {
      if (foundEntrance && foundExit)
        break;
      const vec3f p0 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][0]));
      const vec3f p1 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][1]));
      const vec3f p2 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][2]));

      intersectRayTri(origin,
                      direction,
                      p0,
                      p1,
                      p2,
                      foundEntrance,
                      foundExit,
                      intersectedTRange);
    }
    if (foundEntrance && foundExit) {
      intersectedTRange.lower = max(intersectedTRange.lower, rangeLimit.lower);
      intersectedTRange.upper = min(intersectedTRange.upper, rangeLimit.upper);
      return intersectedTRange;
    }
    return box1f(inf, neg_inf);
  }

  inline box1f intersectRayWedge(const vec3f &origin,
                                 const vec3f &direction,
                                 const box1f &rangeLimit,
                                 const VKLUnstructuredVolume *volume,
                                 const uint64 cellID)
  {
    // Get cell offset in index buffer
    const uint64 cOffset = getCellOffset(volume, cellID);

    const uint64 planes[8][3] = {{cOffset + 2, cOffset + 0, cOffset + 1},
                                 {cOffset + 4, cOffset + 1, cOffset + 0},
                                 {cOffset + 5, cOffset + 2, cOffset + 1},
                                 {cOffset + 5, cOffset + 3, cOffset + 0},
                                 {cOffset + 5, cOffset + 4, cOffset + 3},
                                 {cOffset + 4, cOffset + 0, cOffset + 3},
                                 {cOffset + 5, cOffset + 1, cOffset + 4},
                                 {cOffset + 5, cOffset + 0, cOffset + 2}};
    box1f intersectedTRange   = box1f(neg_inf, inf);
    bool foundExit            = false;
    bool foundEntrance        = false;
    for (int i = 0; i < 8; ++i) {
      if (foundEntrance && foundExit)
        break;
      const vec3f p0 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][0]));
      const vec3f p1 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][1]));
      const vec3f p2 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][2]));

      intersectRayTri(origin,
                      direction,
                      p0,
                      p1,
                      p2,
                      foundEntrance,
                      foundExit,
                      intersectedTRange);
    }
    if (foundEntrance && foundExit) {
      intersectedTRange.lower = max(intersectedTRange.lower, rangeLimit.lower);
      intersectedTRange.upper = min(intersectedTRange.upper, rangeLimit.upper);
      return intersectedTRange;
    }
    return box1f(inf, neg_inf);
  }

  inline box1f intersectRayPyramid(const vec3f &origin,
                                   const vec3f &direction,
                                   const box1f &rangeLimit,
                                   const VKLUnstructuredVolume *volume,
                                   const uint64 cellID)
  {
    // Get cell offset in index buffer
    const uint64 cOffset = getCellOffset(volume, cellID);

    const uint64 planes[6][3] = {{cOffset + 3, cOffset + 0, cOffset + 1},
                                 {cOffset + 4, cOffset + 1, cOffset + 0},
                                 {cOffset + 4, cOffset + 2, cOffset + 1},
                                 {cOffset + 4, cOffset + 3, cOffset + 2},
                                 {cOffset + 3, cOffset + 4, cOffset + 0},
                                 {cOffset + 3, cOffset + 1, cOffset + 2}};
    box1f intersectedTRange   = box1f(neg_inf, inf);
    bool foundExit            = false;
    bool foundEntrance        = false;
    for (int i = 0; i < 6; ++i) {
      if (foundEntrance && foundExit)
        break;
      const vec3f p0 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][0]));
      const vec3f p1 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][1]));
      const vec3f p2 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][2]));

      intersectRayTri(origin,
                      direction,
                      p0,
                      p1,
                      p2,
                      foundEntrance,
                      foundExit,
                      intersectedTRange);
    }
    if (foundEntrance && foundExit) {
      intersectedTRange.lower = max(intersectedTRange.lower, rangeLimit.lower);
      intersectedTRange.upper = min(intersectedTRange.upper, rangeLimit.upper);
      return intersectedTRange;
    }
    return box1f(inf, neg_inf);
  }

  inline box1f intersectRayHex(const vec3f &origin,
                               const vec3f &direction,
                               const box1f &rangeLimit,
                               const VKLUnstructuredVolume *volume,
                               const uint64 cellID)
  {
    // Get cell offset in index buffer
    const uint64 cOffset = getCellOffset(volume, cellID);

    const uint64 planes[12][3] = {{cOffset + 0, cOffset + 1, cOffset + 2},
                                  {cOffset + 0, cOffset + 2, cOffset + 3},
                                  {cOffset + 5, cOffset + 6, cOffset + 1},
                                  {cOffset + 6, cOffset + 2, cOffset + 1},
                                  {cOffset + 7, cOffset + 4, cOffset + 0},
                                  {cOffset + 7, cOffset + 0, cOffset + 3},
                                  {cOffset + 4, cOffset + 5, cOffset + 1},
                                  {cOffset + 4, cOffset + 1, cOffset + 0},
                                  {cOffset + 5, cOffset + 4, cOffset + 6},
                                  {cOffset + 6, cOffset + 4, cOffset + 7},
                                  {cOffset + 6, cOffset + 7, cOffset + 2},
                                  {cOffset + 2, cOffset + 7, cOffset + 3}};
    box1f intersectedTRange    = box1f(neg_inf, inf);
    bool foundExit             = false;
    bool foundEntrance         = false;
    for (int i = 0; i < 12; ++i) {
      if (foundEntrance && foundExit)
        break;
      const vec3f p0 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][0]));
      const vec3f p1 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][1]));
      const vec3f p2 =
          get_vec3f(volume->vertex, getVertexId(volume, planes[i][2]));

      intersectRayTri(origin,
                      direction,
                      p0,
                      p1,
                      p2,
                      foundEntrance,
                      foundExit,
                      intersectedTRange);
    }
    if (foundEntrance && foundExit) {
      intersectedTRange.lower = max(intersectedTRange.lower, rangeLimit.lower);
      intersectedTRange.upper = min(intersectedTRange.upper, rangeLimit.upper);
      return intersectedTRange;
    }
    return box1f(inf, neg_inf);
  }

  inline box1f intersectRayCell(const vec3f &origin,
                                const vec3f &direction,
                                const box1f &rangeLimit,
                                const VKLUnstructuredVolume *volume,
                                const uint64 cellID)
  {
    box1f intersectedTRange;
    switch (get_uint8(volume->cellType, cellID)) {
    case VKL_TETRAHEDRON:
      intersectedTRange =
          intersectRayTet(origin, direction, rangeLimit, volume, cellID);
      break;
    case VKL_HEXAHEDRON:
      intersectedTRange =
          intersectRayHex(origin, direction, rangeLimit, volume, cellID);
      break;
    case VKL_WEDGE:
      intersectedTRange =
          intersectRayWedge(origin, direction, rangeLimit, volume, cellID);
      break;
    case VKL_PYRAMID:
      intersectedTRange =
          intersectRayPyramid(origin, direction, rangeLimit, volume, cellID);
      break;
    }

    box1f result;
    result.lower = max(intersectedTRange.lower, rangeLimit.lower);
    result.upper = min(intersectedTRange.upper, rangeLimit.upper);
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////
  // traversal ///////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  inline Node *sibling(Node *node)
  {
    InnerNode *parent = (InnerNode *)node->parent;

    if (parent->children[0] == node) {
      return parent->children[1];
    } else {
      return parent->children[0];
    }
  }

  inline void intersectNode(const vec3f &origin,
                            const vec3f &direction,
                            const box1f &tRange,
                            const ValueRanges &valueRanges,
                            const box3f &nodeBounds,
                            const box1f &nodeValueRange,
                            box1f &intersectedTRange,
                            bool &isIntersected)
  {
    if (!valueRangesOverlap(valueRanges, nodeValueRange)) {
      intersectedTRange = box1f(inf, neg_inf);
    } else {
      const box3f reduced = box3f(nodeBounds.lower, nodeBounds.upper);
      intersectedTRange   = intersectBox(origin, direction, reduced, tRange);
    }

    isIntersected = !(intersectedTRange.upper <= intersectedTRange.lower);
  }

  inline void intersectLeaf(const vec3f &origin,
                            const vec3f &direction,
                            const box1f &tRange,
                            const ValueRanges &valueRanges,
                            const box1f &nodeValueRange,
                            const VKLUnstructuredVolume *volume,
                            const uint64 cellID,
                            box1f &intersectedTRange,
                            bool &isIntersected)
  {
    if (!valueRangesOverlap(valueRanges, nodeValueRange)) {
      intersectedTRange = box1f(inf, neg_inf);
    } else {
      intersectedTRange =
          intersectRayCell(origin, direction, tRange, volume, cellID);
    }

    isIntersected = !(intersectedTRange.upper <= intersectedTRange.lower);
  }

  inline box1f evalNodeStacklessV(const UnstructuredIntervalIterator *iterator,
                                  const ValueRanges &valueRanges,
                                  const bool elementaryCellIteration,
                                  Node *node,
                                  uint32 bitstack,
                                  box1f hitTRange,
                                  UnstructuredTraversalState &hitState)
  {
    const SamplerShared *sampler = (const SamplerShared *)iterator->sampler;

    while (1) {
      bool isInner = (node->nominalLength.x >= 0);

      if (isInner &&
          (elementaryCellIteration ||
           node->level < iterator->super.context->super.maxIteratorDepth)) {
        InnerNode *inner = (InnerNode *)node;

        box1f intersectedTRange0;
        bool in0;
        intersectNode(iterator->origin,
                      iterator->direction,
                      box1f(iterator->tRange.lower, hitTRange.lower),
                      valueRanges,
                      box3f(inner->bounds[0].lower, inner->bounds[0].upper),
                      box1f(node->valueRange.lower, node->valueRange.upper),
                      intersectedTRange0,
                      in0);

        box1f intersectedTRange1;
        bool in1;
        intersectNode(iterator->origin,
                      iterator->direction,
                      box1f(iterator->tRange.lower, hitTRange.lower),
                      valueRanges,
                      box3f(inner->bounds[1].lower, inner->bounds[1].upper),
                      box1f(node->valueRange.lower, node->valueRange.upper),
                      intersectedTRange1,
                      in1);

        if (in0 || in1) {
          if (in0 && in1) {
            if (intersectedTRange0.lower <= intersectedTRange1.lower) {
              node     = inner->children[0];
              bitstack = (bitstack << 1) | 1;
              continue;
            } else {
              node     = inner->children[1];
              bitstack = (bitstack << 1) | 1;
              continue;
            }
          } else {
            if (in0) {
              node     = inner->children[0];
              bitstack = bitstack << 1;
              continue;
            } else {
              node     = inner->children[1];
              bitstack = bitstack << 1;
              continue;
            }
          }

          // should never reach this point
          return box1f(inf, neg_inf);
        }
      } else {
        // leaf, or at maximum traversal level
        box1f intersectedTRange;
        bool in0;

        if (isInner) {
          InnerNode *inner = (InnerNode *)node;
          box3f bounds =
              box_extend(box3f(inner->bounds[0].lower, inner->bounds[0].upper),
                         box3f(inner->bounds[1].lower, inner->bounds[1].upper));
          intersectNode(iterator->origin,
                        iterator->direction,
                        iterator->tRange,  // use full tRange to capture full
                                           // ray / node overlap (not clipped by
                                           // previous leaf intersection)
                        valueRanges,
                        bounds,
                        box1f(node->valueRange.lower, node->valueRange.upper),
                        intersectedTRange,
                        in0);
        } else {
          LeafNode *leaf = (LeafNode *)node;

          if (elementaryCellIteration) {
            // only supported for leaf node's with one cell
            const VKLUnstructuredVolume *volume =
                (const VKLUnstructuredVolume *)
                    iterator->super.context->super.sampler->volume;
            intersectLeaf(iterator->origin,
                          iterator->direction,
                          iterator->tRange,  // use full tRange to capture full
                                             // ray / node overlap (not clipped
                                             // by previous leaf intersection)
                          valueRanges,
                          box1f(node->valueRange.lower, node->valueRange.upper),
                          volume,
                          ((LeafNodeSingle *)leaf)->cellID,
                          intersectedTRange,
                          in0);
          } else {
            box3f bounds = box3f(leaf->bounds.lower, leaf->bounds.upper);
            intersectNode(iterator->origin,
                          iterator->direction,
                          iterator->tRange,  // use full tRange to capture full
                                             // ray / node overlap (not clipped
                                             // by previous leaf intersection)
                          valueRanges,
                          bounds,
                          box1f(node->valueRange.lower, node->valueRange.upper),
                          intersectedTRange,
                          in0);
          }
        }

        if (in0) {
          if (intersectedTRange.lower < hitTRange.lower) {
            if (hitTRange.lower == inf) {
              // first hit; we will restart from here, since we may skip farther
              // nodes before later returning a closer node (which would lead to
              // holes with a later restart point)
              hitState.node     = node;
              hitState.bitstack = bitstack;
            }

            hitTRange = intersectedTRange;
          }
        }
      }

      // backtrack
      while ((bitstack & 1) == 0) {
        if (!bitstack) {
          return hitTRange;
        }

        node     = node->parent;
        bitstack = bitstack >> 1;
      }

      // sibling pointer could be used here if we had it
      node = sibling(node);

      bitstack = bitstack ^ 1;
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  // API entrypoints /////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  inline void UnstructuredIntervalIterator_Init(
      UnstructuredIntervalIterator *self,
      const IntervalIteratorContext *context,
      const vec3f *origin,
      const vec3f *direction,
      const box1f *tRange,
      const bool elementaryCellIterationSupported)
  {
    self->super.context                    = context;
    self->elementaryCellIterationSupported = elementaryCellIterationSupported;

    self->sampler   = self->super.context->super.sampler;
    self->origin    = *origin;
    self->direction = *direction;
    self->tRange    = *tRange;

    const VKLUnstructuredBase *volume =
        (const VKLUnstructuredBase *)self->super.context->super.sampler->volume;

    self->traversalState.node     = volume->bvhRoot;
    self->traversalState.bitstack = 0;
  }

  inline void UnstructuredIntervalIterator_iterateInternal(
      UnstructuredIntervalIterator *self,
      Interval *interval,
      const ValueRanges &valueRanges,
      const bool elementaryCellIteration,
      int *result)
  {
    UnstructuredTraversalState hitState;
    hitState.node     = NULL;
    hitState.bitstack = 0;

#if 1
    // with restart
    UnstructuredTraversalState *startState =
        (UnstructuredTraversalState *)&self->traversalState;

    box1f retRange;

    Node *node        = startState->node;
    uint32_t bitstack = startState->bitstack;

    retRange = evalNodeStacklessV(self,
                                  valueRanges,
                                  elementaryCellIteration,
                                  node,
                                  bitstack,
                                  box1f(inf, inf),
                                  hitState);
#else
    // without restart
    const VKLUnstructuredBase *volume =
        (const VKLUnstructuredBase *)self->sampler->volume;

    box1f retRange = evalNodeStacklessV(self,
                                        valueRanges,
                                        false /*elementaryCellIteration*/,
                                        volume->bvhRoot,
                                        0,
                                        box1f(inf, inf),
                                        hitState);
#endif

    if (retRange.lower == inf || isEmpty(retRange)) {
      *result = false;
    } else {
      self->tRange.lower = retRange.upper;

      self->traversalState = *((UnstructuredTraversalState *)&hitState);

      interval->tRange.lower     = retRange.lower;
      interval->tRange.upper     = retRange.upper;
      interval->valueRange.lower = hitState.node->valueRange.lower;
      interval->valueRange.upper = hitState.node->valueRange.upper;
      interval->nominalDeltaT =
          reduce_min(absf(hitState.node->nominalLength *
                          vec3f(rcp_safe(self->direction.x),
                                rcp_safe(self->direction.y),
                                rcp_safe(self->direction.z))));  // in ray space
      *result = true;
    }
  }

  inline void UnstructuredIntervalIterator_iterate(
      UnstructuredIntervalIterator *self, Interval *interval, int *result)
  {
    // We specify elementaryCellIteration in the call here, as it may
    // be toggled for public API interval iteration via a sampler parameter.
    // It is however typically enabled by default in hit iteration (if
    // supported), which also calls iterateInterval(). This is why we don't
    // simply detect the flag inside the interval iteration function itself.
    //
    // Also, some volume types use the unstructured iterator but do not support
    // elementary cell intersections; in those case elementary iteration will
    // not be used.
    UnstructuredIntervalIterator_iterateInternal(
        self,
        interval,
        self->super.context->super.valueRanges,
        self->elementaryCellIterationSupported &&
            self->super.context->super.elementaryCellIteration,
        result);
  }

}  // namespace ispc
