// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "UnstructuredIterator.ih"
#include "common/export_util.h"
#include "common/print_debug.ih"
#include "math/box_utility.ih"
#include "volume/UnstructuredVolume.ih"

// Ignore warning about exporting uniform-pointer-to-varying, as this is in
// fact legal.
#pragma ignore warning(all)
export void EXPORT_UNIQUE(UnstructuredIterator_export,
                          uniform vec3i &dummy_vec3i,
                          uniform vec3f &dummy_vec3f,
                          uniform box1f &dummy_box1f,
                          const varying UnstructuredIterator *uniform it)
{
}

export uniform int EXPORT_UNIQUE(UnstructuredIterator_sizeOf)
{
  return sizeof(varying UnstructuredIterator);
}

void UnstructuredIterator_iterateIntervalInternal(
    const int *uniform imask,
    void *uniform _self,
    void *uniform _interval,
    const uniform ValueRanges &valueRanges,
    const uniform bool elementaryCellIteration,
    uniform int *uniform _result);

export void EXPORT_UNIQUE(UnstructuredIterator_Initialize,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _context,
                          void *uniform _origin,
                          void *uniform _direction,
                          void *uniform _tRange,
                          const uniform bool elementaryCellIterationSupported)
{
  if (!imask[programIndex]) {
    return;
  }

  varying UnstructuredIterator *uniform self =
      (varying UnstructuredIterator * uniform) _self;

  self->super.context = (const IntervalIteratorContext *uniform)_context;
  self->super.iterate = UnstructuredIterator_iterateIntervalInternal;
  self->super.elementaryCellIterationSupported =
      elementaryCellIterationSupported;

  self->sampler =
      (UnstructuredSamplerBase * uniform)(self->super.context->sampler);
  self->origin    = *((varying vec3f * uniform) _origin);
  self->direction = *((varying vec3f * uniform) _direction);
  self->tRange    = *((varying box1f * uniform) _tRange);

  const VKLUnstructuredBase *uniform volume =
      (const VKLUnstructuredBase *uniform)self->super.context->sampler->volume;

  self->traversalState.node     = (uint64)(volume->bvhRoot);
  self->traversalState.bitstack = 0;
}

static inline bool disjoint(uniform box1f a, varying box1f b)
{
  return (a.upper < b.lower) || (b.upper < a.lower);
}

#define template_intersectNode(univary)                                       \
  inline void intersectNode_##univary(const univary vec3f &origin,            \
                                      const univary vec3f &direction,         \
                                      const univary box1f &tRange,            \
                                      const uniform ValueRanges &valueRanges, \
                                      const uniform box3fa &nodeBounds,       \
                                      const uniform box1f &nodeValueRange,    \
                                      univary box1f &intersectedTRange,       \
                                      univary bool &isIntersected)            \
  {                                                                           \
    if (!valueRangesOverlap(valueRanges, nodeValueRange)) {                   \
      intersectedTRange = make_box1f(inf, neg_inf);                           \
    } else {                                                                  \
      const uniform box3f reduced =                                           \
          make_box3f(nodeBounds.lower, nodeBounds.upper);                     \
      intersectedTRange = intersectBox(origin, direction, reduced, tRange);   \
    }                                                                         \
                                                                              \
    isIntersected = !(intersectedTRange.upper <= intersectedTRange.lower);    \
  }

// template_intersectNode(uniform);
template_intersectNode(varying);
#undef template_intersectNode

#define template_intersectLeaf(univary)                                    \
  inline void intersectLeaf_##univary(                                     \
      const univary vec3f &origin,                                         \
      const univary vec3f &direction,                                      \
      const univary box1f &tRange,                                         \
      const uniform ValueRanges &valueRanges,                              \
      const uniform box1f &nodeValueRange,                                 \
      const VKLUnstructuredVolume *uniform volume,                         \
      const uniform uint64 cellID,                                         \
      univary box1f &intersectedTRange,                                    \
      univary bool &isIntersected)                                         \
  {                                                                        \
    if (!valueRangesOverlap(valueRanges, nodeValueRange)) {                \
      intersectedTRange = make_box1f(inf, neg_inf);                        \
    } else {                                                               \
      intersectedTRange = intersectRayCell_##univary(                      \
          origin, direction, tRange, volume, cellID);                      \
    }                                                                      \
                                                                           \
    isIntersected = !(intersectedTRange.upper <= intersectedTRange.lower); \
  }

// template_intersectNode(uniform);
template_intersectLeaf(varying);
#undef template_intersectLeaf

static box1f evalNodeStacklessV(const varying UnstructuredIterator *uniform
                                    iterator,
                                const uniform ValueRanges &valueRanges,
                                const uniform bool elementaryCellIteration,
                                uniform Node *uniform node,
                                uniform uint32 bitstack,
                                box1f hitTRange,
                                UnstructuredTraversalState &hitState)
{
  const UnstructuredSamplerBase *uniform sampler =
      (const UnstructuredSamplerBase *uniform)iterator->super.context->sampler;

  while (1) {
    uniform bool isInner = (node->nominalLength.x >= 0);

    if (isInner && (elementaryCellIteration ||
                    node->level < iterator->super.context->maxIteratorDepth)) {
      uniform InnerNode *uniform inner = (uniform InnerNode * uniform) node;

      box1f intersectedTRange0;
      bool in0;
      intersectNode_varying(iterator->origin,
                            iterator->direction,
                            make_box1f(iterator->tRange.lower, hitTRange.lower),
                            valueRanges,
                            inner->bounds[0],
                            node->valueRange,
                            intersectedTRange0,
                            in0);

      box1f intersectedTRange1;
      bool in1;
      intersectNode_varying(iterator->origin,
                            iterator->direction,
                            make_box1f(iterator->tRange.lower, hitTRange.lower),
                            valueRanges,
                            inner->bounds[1],
                            node->valueRange,
                            intersectedTRange1,
                            in1);

      if (in0 || in1) {
        // bitstack = bitstack << 1;

        if (in0 && in1) {
          if (intersectedTRange0.lower <= intersectedTRange1.lower) {
            // node = inner->children[0];
            return evalNodeStacklessV(iterator,
                                      valueRanges,
                                      elementaryCellIteration,
                                      inner->children[0],
                                      (bitstack << 1) | 1,
                                      hitTRange,
                                      hitState);
          } else {
            // node = inner->children[1];
            return evalNodeStacklessV(iterator,
                                      valueRanges,
                                      elementaryCellIteration,
                                      inner->children[1],
                                      (bitstack << 1) | 1,
                                      hitTRange,
                                      hitState);
          }

          // bitstack = bitstack | 1;
        } else {
          if (in0) {
            // node = inner->children[0];
            return evalNodeStacklessV(iterator,
                                      valueRanges,
                                      elementaryCellIteration,
                                      inner->children[0],
                                      bitstack << 1,
                                      hitTRange,
                                      hitState);
          } else {
            // node = inner->children[1];
            return evalNodeStacklessV(iterator,
                                      valueRanges,
                                      elementaryCellIteration,
                                      inner->children[1],
                                      bitstack << 1,
                                      hitTRange,
                                      hitState);
          }
        }

        // should never reach this point
        return make_box1f(inf, neg_inf);
      }
    } else {
      // leaf, or at maximum traversal level
      box1f intersectedTRange;
      bool in0;

      if (isInner) {
        uniform InnerNode *uniform inner = (uniform InnerNode * uniform) node;
        uniform box3fa bounds = box_extend(inner->bounds[0], inner->bounds[1]);
        intersectNode_varying(
            iterator->origin,
            iterator->direction,
            iterator
                ->tRange,  // use full tRange to capture full ray / node overlap
                           // (not clipped by previous leaf intersection)
            valueRanges,
            bounds,
            node->valueRange,
            intersectedTRange,
            in0);
      } else {
        uniform LeafNode *uniform leaf = (uniform LeafNode * uniform) node;

        if (elementaryCellIteration) {
          // only supported for leaf node's with one cell
          const VKLUnstructuredVolume *uniform volume =
              (const VKLUnstructuredVolume *uniform)
                  iterator->super.context->sampler->volume;
          intersectLeaf_varying(
              iterator->origin,
              iterator->direction,
              iterator->tRange,  // use full tRange to capture full ray / node
                                 // overlap (not clipped by previous leaf
                                 // intersection)
              valueRanges,
              node->valueRange,
              volume,
              ((uniform LeafNodeSingle * uniform) leaf)->cellID,
              intersectedTRange,
              in0);
        } else {
          uniform box3fa bounds = leaf->bounds;
          intersectNode_varying(
              iterator->origin,
              iterator->direction,
              iterator->tRange,  // use full tRange to capture full ray / node
                                 // overlap (not clipped by previous leaf
                                 // intersection)
              valueRanges,
              bounds,
              node->valueRange,
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

inline void UnstructuredIterator_iterateIntervalInternal(
    const int *uniform imask,
    void *uniform _self,
    void *uniform _interval,
    const uniform ValueRanges &valueRanges,
    const uniform bool elementaryCellIteration,
    uniform int *uniform _result)
{
  if (!imask[programIndex]) {
    return;
  }

  varying UnstructuredIterator *uniform self =
      (varying UnstructuredIterator * uniform) _self;

  varying Interval *uniform interval = (varying Interval * uniform) _interval;

  varying int *uniform result = (varying int *uniform)_result;

  UnstructuredTraversalState hitState;
  hitState.node     = NULL;
  hitState.bitstack = 0;

#if 1
  // with restart

  varying UnstructuredTraversalState *uniform startState =
      (varying UnstructuredTraversalState * uniform) & self->traversalState;

  box1f retRange;

  foreach_unique(node in startState->node)
  {
    foreach_unique(bitstack in startState->bitstack)
    {
      retRange = evalNodeStacklessV(self,
                                    valueRanges,
                                    elementaryCellIteration,
                                    node,
                                    bitstack,
                                    make_box1f(inf, inf),
                                    hitState);
    }
  }
#else
  // without restart
  const VKLUnstructuredBase *uniform volume =
      (const VKLUnstructuredBase *uniform)self->sampler->super.volume;

  box1f retRange = evalNodeStacklessV(self,
                                      valueRanges,
                                      elementaryCellIteration,
                                      volume->bvhRoot,
                                      0,
                                      make_box1f(inf, inf),
                                      hitState);
#endif

  if (retRange.lower == inf || isEmpty(retRange)) {
    *result = false;
    PRINT_DEBUG("Empty range\n");
  } else {
    self->tRange.lower = retRange.upper;

    self->traversalState =
        *((varying UnstructuredTraversalStatePublic * uniform) & hitState);

    interval->tRange.lower     = retRange.lower;
    interval->tRange.upper     = retRange.upper;
    interval->valueRange.lower = hitState.node->valueRange.lower;
    interval->valueRange.upper = hitState.node->valueRange.upper;
    interval->nominalDeltaT =
        reduce_min(absf(hitState.node->nominalLength *
                        rcp_safe(self->direction)));  // in ray space
    *result = true;
  }
}

export void EXPORT_UNIQUE(UnstructuredIterator_iterateInterval,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _interval,
                          uniform int *uniform _result)
{
  varying UnstructuredIterator *uniform self =
      (varying UnstructuredIterator * uniform) _self;

  // We specify elementaryCellIteration in the call here, as it may
  // be toggled for public API interval iteration via a sampler parameter.
  // It is however typically enabled by default in hit iteration (if supported),
  // which also calls iterateInterval(). This is why we don't simply detect the
  // flag inside the interval iteration function itself.
  //
  // Also, some volume types use the unstructured iterator but do not support
  // elementary cell intersections; in those case elementary iteration will not
  // be used.
  UnstructuredIterator_iterateIntervalInternal(
      imask,
      _self,
      _interval,
      self->super.context->valueRanges,
      self->super.elementaryCellIterationSupported &&
          self->super.context->elementaryCellIteration,
      _result);
}
