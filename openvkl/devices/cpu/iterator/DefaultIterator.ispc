// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "../math/box_utility.ih"
#include "DefaultIterator.ih"
#include "common/ValueRanges.ih"

// Ignore warning about exporting uniform-pointer-to-varying, as this is in
// fact legal.
#pragma ignore warning(all)
export void EXPORT_UNIQUE(DefaultIterator_export,
                          uniform vec3f &dummy_vec3f,
                          const varying DefaultIntervalIterator *uniform
                              itInterval,
                          const varying DefaultHitIterator *uniform itHit)
{
}

void DefaultIntervalIterator_iterateIntervalInternal(
    const int *uniform imask,
    void *uniform _self,
    void *uniform _interval,
    const uniform ValueRanges &valueRanges,
    const uniform bool /*elementaryCellIteration*/,
    uniform int *uniform _result);

export void EXPORT_UNIQUE(DefaultIntervalIterator_Initialize,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _context,
                          void *uniform _origin,
                          void *uniform _direction,
                          void *uniform _tRange,
                          const uniform box3f &boundingBox,
                          const uniform box1f &valueRange)
{
  if (!imask[programIndex]) {
    return;
  }

  varying DefaultIntervalIterator *uniform self =
      (varying DefaultIntervalIterator * uniform) _self;

  self->super.context = (const IntervalIteratorContext *uniform)_context;
  self->super.iterate = DefaultIntervalIterator_iterateIntervalInternal;
  self->super.elementaryCellIterationSupported = false;

  const vec3f origin    = *((varying vec3f * uniform) _origin);
  const vec3f direction = *((varying vec3f * uniform) _direction);
  self->valueRange      = valueRange;

  box1f tRange = *((varying box1f * uniform) _tRange);
  self->boundingBoxTRange =
      intersectBox(origin, direction, boundingBox, tRange);

  // compute a nominal interval length as a fraction of the largest bounding box
  // dimension, in ray space
  uniform float bbMaxDimension =
      reduce_max(boundingBox.upper - boundingBox.lower);
  self->nominalIntervalLength = 0.1f * bbMaxDimension * rcp(length(direction));

  resetInterval(self->currentInterval);
}

inline void DefaultIntervalIterator_iterateIntervalInternal(
    const int *uniform imask,
    void *uniform _self,
    void *uniform _interval,
    const uniform ValueRanges &valueRanges,
    const uniform bool /*elementaryCellIteration*/,
    uniform int *uniform _result)
{
  if (!imask[programIndex]) {
    return;
  }

  varying DefaultIntervalIterator *uniform self =
      (varying DefaultIntervalIterator * uniform) _self;

  varying Interval *uniform interval = (varying Interval * uniform) _interval;

  varying int *uniform result = (varying int *uniform)_result;

  if (isempty1f(self->boundingBoxTRange)) {
    *result = false;
    return;
  }

  if (!valueRangesOverlap(valueRanges, self->valueRange)) {
    *result = false;
    return;
  }

  Interval nextInterval;

  nextInterval.tRange.lower =
      max(self->currentInterval.tRange.upper, self->boundingBoxTRange.lower);
  nextInterval.tRange.upper =
      min(nextInterval.tRange.lower + self->nominalIntervalLength,
          self->boundingBoxTRange.upper);

  if (nextInterval.tRange.upper <= nextInterval.tRange.lower) {
    *result = false;
    return;
  }

  // conservatively use the volume value range
  nextInterval.valueRange    = self->valueRange;
  nextInterval.nominalDeltaT = 0.25f * self->nominalIntervalLength;

  self->currentInterval = nextInterval;
  *interval             = nextInterval;
  *result               = true;
}

export void EXPORT_UNIQUE(DefaultIntervalIterator_iterateInterval,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _interval,
                          uniform int *uniform _result)
{
  varying DefaultIntervalIterator *uniform self =
      (varying DefaultIntervalIterator * uniform) _self;

  DefaultIntervalIterator_iterateIntervalInternal(
      imask,
      _self,
      _interval,
      self->super.context->super.valueRanges,
      false,
      _result);
}

// -----------------------------------------------------------------------------

export void EXPORT_UNIQUE(DefaultHitIterator_Initialize,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _intervalIterator,
                          void *uniform _context,
                          void *uniform _origin,
                          void *uniform _direction,
                          void *uniform _time)
{
  if (!imask[programIndex]) {
    return;
  }

  varying DefaultHitIterator *uniform self =
      (varying DefaultHitIterator * uniform) _self;

  self->context               = (const HitIteratorContext *uniform)_context;
  self->origin                = *((varying vec3f * uniform) _origin);
  self->direction             = *((varying vec3f * uniform) _direction);
  self->time                  = *((varying float *uniform)_time);
  self->intervalIteratorState = _intervalIterator;
  self->lastHitOffsetT        = neg_inf;

  // Reset to invalid.
  resetInterval(self->currentInterval);
}

export void EXPORT_UNIQUE(DefaultHitIterator_iterateHit,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _hit,
                          uniform int *uniform _result)
{
  if (!imask[programIndex])
    return;

  varying int *uniform result = (varying int *uniform)_result;
  *result                     = false;

  varying DefaultHitIterator *uniform self =
      (varying DefaultHitIterator * uniform) _self;

  // The selector prunes everything - don't iterate at all.
  if (self->context->numValues == 0)
    return;

  if (self->currentInterval.tRange.lower == inf)  // Ray has finished already.
    return;

  // Note: For an interval iterator to be compatible with the default hit
  // iterator, it must inherit from DefaultHitIteratorIntervalIterator.
  const DefaultHitIteratorIntervalIterator *uniform intervalIterator =
      (DefaultHitIteratorIntervalIterator * uniform)
          self->intervalIteratorState;

  // Generate intervals using the interval iterator until we either run out
  // or find an isosurface hit.

  // We enable elementary cell iteration for more robust bracketing intervals,
  // if the underlying interval iterator supports it.
  const uniform bool elementaryCellIteration =
      intervalIterator->elementaryCellIterationSupported;

  for (uniform int i = 0;; ++i) {
    const int needInterval = isempty1f(self->currentInterval.tRange);
    int haveInterval       = !needInterval;
    intervalIterator->iterate((const int *uniform) & needInterval,
                              self->intervalIteratorState,
                              &self->currentInterval,
                              self->context->super.super.valueRanges,
                              elementaryCellIteration,
                              (int *uniform) & haveInterval);

    // We've found a new interval, but it's possible the last hit may have moved
    // us past the beginning of this new interval. This can occur with hits on
    // interval boundaries, for example. We also need to check if this nudges us
    // out of the current interval.
    if (needInterval && haveInterval) {
      self->currentInterval.tRange.lower =
          max(self->currentInterval.tRange.lower, self->lastHitOffsetT);

      if (isempty1f(self->currentInterval.tRange)) {
        continue;
      }
    }

    if (!haveInterval) {
      // We use this to indicate that this lane has run out of intervals,
      // so that we can use an early exit on the next iterateHit call.
      self->currentInterval.tRange.lower = inf;
      return;
    }

    varying Hit *uniform hit = (varying Hit * uniform) _hit;
    hit->t                   = inf;
    bool foundHit =
        intersectSurfacesBisection(self->context->super.super.sampler,
                                   self->origin,
                                   self->direction,
                                   self->currentInterval.tRange,
                                   self->context->super.super.attributeIndex,
                                   self->time,
                                   self->currentInterval.nominalDeltaT,
                                   self->context->numValues,
                                   self->context->values,
                                   *hit);

    *result |= foundHit;

    if (foundHit) {
      self->currentInterval.tRange.lower = hit->t + hit->epsilon;
      self->lastHitOffsetT               = hit->t + hit->epsilon;
      return;
    }

    self->currentInterval.tRange.lower = inf;
  }
}
