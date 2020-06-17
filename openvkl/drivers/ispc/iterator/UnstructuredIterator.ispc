// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "UnstructuredIterator.ih"
#include "common/export_util.h"
#include "common/print_debug.ih"
#include "math/box_utility.ih"
#include "value_selector/ValueSelector.ih"
#include "volume/UnstructuredVolume.ih"

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
                          const uniform box1f &valueRange,
                          uniform int *uniform _result);

export void EXPORT_UNIQUE(UnstructuredIterator_Initialize,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _volume,
                          void *uniform _origin,
                          void *uniform _direction,
                          void *uniform _tRange,
                          void *uniform _valueSelector)
{
  if (!imask[programIndex]) {
    return;
  }

  varying UnstructuredIterator *uniform self =
      (varying UnstructuredIterator * uniform) _self;
  self->iterateInterval = UnstructuredIterator_iterateIntervalInternal;

  self->volume        = (VKLUnstructuredVolume * uniform) _volume;
  self->origin        = *((varying vec3f * uniform) _origin);
  self->direction     = *((varying vec3f * uniform) _direction);
  self->tRange        = *((varying box1f * uniform) _tRange);
  self->valueSelector = (uniform ValueSelector * uniform) _valueSelector;
  self->getCount      = 0;
}

// How deep in the BVH we're going to look for intervals.
// This indirectly determines how tight the bounds might be,
// as currently we just return a single interval.
#define MAX_LEVEL 6

static inline bool disjoint(uniform box1f a, varying box1f b)
{
  return (a.upper < b.lower) || (b.upper < a.lower);
}

static inline box1f make_box1f_empty()
{
  return make_box1f(inf, neg_inf);
}

static box1f evalNode(varying UnstructuredIterator *uniform iterator,
                      uniform Node *uniform node,
                      const uniform box1f &valueRange,
                      box1f &outputValueRange,
                      float &deltaT,
                      uniform int level)
{
  PRINT_DEBUG("ispc: % %\n", level, node);

  // rejection based on values being in the range we're looking for
  if (disjoint(valueRange, node->valueRange)) {
    PRINT_DEBUG("rejected range:\n\t%\n\t%\n",
                node->valueRange.lower,
                node->valueRange.upper);
    outputValueRange = make_box1f_empty();
    deltaT     = inf;
    return make_box1f_empty();
  }

  uniform bool isLeaf = (node->nominalLength < 0);
  uniform box3fa box;
  if (isLeaf) {
    uniform LeafNode *uniform leaf = (uniform LeafNode * uniform) node;
    box                            = leaf->bounds;
  } else {
    uniform InnerNode *uniform inner = (uniform InnerNode * uniform) node;
    box = box_extend(inner->bounds[0], inner->bounds[1]);
  }

  uniform box3f reduced = make_box3f(box.lower, box.upper);
  range1f nodeTRange    = intersectBox(
      iterator->origin, iterator->direction, reduced, iterator->tRange);

  PRINT_DEBUG("box dimensions:\n");
  PRINT_DEBUG("\tlower:\n\t\t%\n\t\t%\n\t\t%\n",
              reduced.lower.x,
              reduced.lower.y,
              reduced.lower.z);
  PRINT_DEBUG("\tupper:\n\t\t%\n\t\t%\n\t\t%\n",
              reduced.upper.x,
              reduced.upper.y,
              reduced.upper.z);
  PRINT_DEBUG("box valueRange:\n\t%\n\t%\n",
              node->valueRange.lower,
              node->valueRange.upper);
  PRINT_DEBUG("box tRange:\n\t%\n\t%\n", nodeTRange.lower, nodeTRange.upper);

  // rejection based on ray/box intersection
  if (isEmpty(nodeTRange)) {
    outputValueRange = make_box1f_empty();
    deltaT     = inf;
    return make_box1f_empty();
  } else if (isLeaf || level > MAX_LEVEL) {
    outputValueRange = node->valueRange;
    deltaT     = abs(node->nominalLength);
    return nodeTRange;
  }

  // going to be an inner node if we reach here
  uniform InnerNode *uniform inner = (uniform InnerNode * uniform) node;
  box1f valueRangeFirst, valueRangeSecond;
  float deltaT1, deltaT2;
  range1f tRangeFirst = evalNode(
      iterator, inner->children[0], valueRange, valueRangeFirst, deltaT1, level + 1);
  range1f tRangeSecond = evalNode(
      iterator, inner->children[1], valueRange, valueRangeSecond, deltaT2, level + 1);
  outputValueRange = box_extend(valueRangeFirst, valueRangeSecond);
  deltaT     = min(deltaT1, deltaT2);
  box1f foo = box_extend(tRangeFirst, tRangeSecond);

  return foo;
}

inline void UnstructuredIterator_iterateIntervalInternal(
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _interval,
                          const uniform box1f &valueRange,
                          uniform int *uniform _result)
{  
  if (!imask[programIndex]) {
    return;
  }

  varying UnstructuredIterator *uniform self =
      (varying UnstructuredIterator * uniform) _self;

  varying Interval *uniform interval = (varying Interval * uniform) _interval;

  varying int *uniform result = (varying int *uniform)_result;

  if (self->getCount != 0) {
    *result = false;
    return;
  }

  box1f intervalValueRange = make_box1f_empty();
  float deltaT;
  const box1f retRange =
      evalNode(self, self->volume->super.bvhRoot, valueRange, intervalValueRange, deltaT, 0);

  if (isEmpty(retRange)) {
    *result = false;
    PRINT_DEBUG("Empty range\n");
  } else {
    // shrink range slightly for cell-valued where testing the boundary will
    // fail (modify test instead?)
    interval->tRange.lower     = retRange.lower + 0.00001;
    interval->tRange.upper     = retRange.upper - 0.00001;
    interval->valueRange.lower = intervalValueRange.lower;
    interval->valueRange.upper = intervalValueRange.upper;
    interval->nominalDeltaT    = deltaT;
    *result                    = true;
  }

  self->getCount++;
}

export void EXPORT_UNIQUE(UnstructuredIterator_iterateInterval,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _interval,
                          uniform int *uniform _result)
{
  varying UnstructuredIterator *uniform self =
      (varying UnstructuredIterator * uniform) _self;

  uniform box1f valueRange;
  if (self->valueSelector)
    valueRange = self->valueSelector->rangesMinMax;
  else {
    valueRange.lower = -inf;
    valueRange.upper = inf;
  }
  UnstructuredIterator_iterateIntervalInternal(
      imask, _self, _interval, valueRange, _result);
}
