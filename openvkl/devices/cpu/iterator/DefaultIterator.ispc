// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "../math/box_utility.ih"
#include "../value_selector/ValueSelector.ih"
#include "../volume/Volume.ih"
#include "DefaultIterator.ih"

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
    const uniform box1f &valueRange,
    const uniform bool /*elementaryCellIteration*/,
    uniform int *uniform _result);

export void EXPORT_UNIQUE(DefaultIntervalIterator_Initialize,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _origin,
                          void *uniform _direction,
                          void *uniform _tRange,
                          void *uniform _valueSelector,
                          const uniform box3f &boundingBox,
                          const uniform box1f &valueRange)
{
  if (!imask[programIndex]) {
    return;
  }

  varying DefaultIntervalIterator *uniform self =
      (varying DefaultIntervalIterator * uniform) _self;

  /* Enable the default hit iterator. */
  self->iterateInterval = DefaultIntervalIterator_iterateIntervalInternal;
  self->elementaryCellIterationSupported = false;

  const vec3f origin    = *((varying vec3f * uniform) _origin);
  const vec3f direction = *((varying vec3f * uniform) _direction);
  self->valueSelector = (uniform ValueSelector * uniform) _valueSelector;
  self->valueRange    = valueRange;

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
    const uniform box1f &valueRange,
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

  if (!overlaps1f(valueRange, self->valueRange)) {
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

  uniform box1f valueRange;
  if (self->valueSelector)
    valueRange = self->valueSelector->rangesMinMax;
  else {
    valueRange.lower = -inf;
    valueRange.upper = inf;
  }
  DefaultIntervalIterator_iterateIntervalInternal(
      imask, _self, _interval, valueRange, false, _result);
}

// -----------------------------------------------------------------------------

export void EXPORT_UNIQUE(DefaultHitIterator_Initialize,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _intervalIterator,
                          void *uniform _sampler,
                          void *uniform _origin,
                          void *uniform _direction,
                          void *uniform _time,
                          void *uniform _valueSelector)
{
  if (!imask[programIndex]) {
    return;
  }

  varying DefaultHitIterator *uniform self =
      (varying DefaultHitIterator * uniform) _self;

  self->sampler       = (Sampler * uniform) _sampler;
  self->valueSelector = (uniform ValueSelector * uniform) _valueSelector;
  self->origin        = *((varying vec3f * uniform) _origin);
  self->direction     = *((varying vec3f * uniform) _direction);
  self->time          = *((varying float *uniform)_time);
  self->intervalIteratorState = _intervalIterator;

  // Reset to invalid.
  resetInterval(self->currentInterval);
}

struct IntervalIteratorT
{
  IterateIntervalFunc iterate;
  uniform bool elementaryCellIterationSupported;
};

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
  if(!self->valueSelector || self->valueSelector->numValues == 0) return;

  if (self->currentInterval.tRange.lower == inf)  // Ray has finished already.
    return;

  // Note: We simply assume the presence of a functor as the first element
  //       in the interval iterator state. This will wreak havoc if that
  //       functor is not present.
  const IntervalIteratorT *uniform intervalIterator =
      (IntervalIteratorT * uniform) self->intervalIteratorState;

  // Generate intervals using the interval iterator until we either run out
  // or find an isosurface hit.

  // We enable elementary cell iteration for more robust bracketing intervals,
  // if the underlying interval iterator supports it. The bisection method can
  // then use this knowledge (i.e. if we have an elementary cell tRange) to
  // further improve robustness of returned hits.
  const uniform bool elementaryCellIteration =
      intervalIterator->elementaryCellIterationSupported;

  for (uniform int i = 0;; ++i) {
    const int needInterval = isempty1f(self->currentInterval.tRange);
    int haveInterval       = !needInterval;
    intervalIterator->iterate((const int *uniform) & needInterval,
                              self->intervalIteratorState,
                              &self->currentInterval,
                              self->valueSelector->valuesMinMax,
                              elementaryCellIteration,
                              (int *uniform) & haveInterval);

    if (!haveInterval) {
      // We use this to indicate that this lane has run out of intervals,
      // so that we can use an early exit on the next iterateHit call.
      self->currentInterval.tRange.lower = inf;
      return;
    }

    // Interval iteration currently occurs on a single value range spanning all
    // potential hit values; this can lead to more intervals than necessary when
    // we have many hit values spread over a wide value range. So we do more
    // specific checks per hit value here to see if we can skip some of these
    // intervals.
    bool intervalOverlapsHitValues = false;

    for (uniform int j = 0; j < self->valueSelector->numValues; j++) {
      const uniform float value = self->valueSelector->values[j];

      if (value >= self->currentInterval.valueRange.lower &&
          value <= self->currentInterval.valueRange.upper) {
        intervalOverlapsHitValues = true;
        break;
      }
    }

    if (intervalOverlapsHitValues) {
      varying Hit *uniform hit = (varying Hit * uniform) _hit;
      hit->t                   = inf;
      bool foundHit =
          intersectSurfacesBisection(self->sampler,
                                     self->origin,
                                     self->direction,
                                     self->currentInterval.tRange,
                                     self->time,
                                     self->currentInterval.nominalDeltaT,
                                     self->valueSelector->numValues,
                                     self->valueSelector->values,
                                     elementaryCellIteration,
                                     *hit);

      *result |= foundHit;

      if (foundHit) {
        self->currentInterval.tRange.lower = hit->t + hit->epsilon;
        return;
      }
    }

    self->currentInterval.tRange.lower = inf;
  }
}
