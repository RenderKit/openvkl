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

void DefaultIntervalIterator_iterateIntervalInternal(const int *uniform imask,
                                                     void *uniform _self,
                                                     void *uniform _interval,
                                                     const uniform box1f &valueRange,
                                                     uniform int *uniform
                                                         _result);

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

  self->iterateInterval = DefaultIntervalIterator_iterateIntervalInternal;
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
      imask, _self, _interval, valueRange, _result);
}

// -----------------------------------------------------------------------------

export void EXPORT_UNIQUE(DefaultHitIterator_Initialize,
                          const int *uniform imask,
                          void *uniform _self,
                          void *uniform _intervalIterator,
                          void *uniform _sampler,
                          void *uniform _origin,
                          void *uniform _direction,
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
  self->intervalIteratorState = _intervalIterator;

  // Reset to invalid.
  resetInterval(self->currentInterval);
}

struct IntervalIteratorT
{
  IterateIntervalFunc iterate;
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

  for (uniform int i = 0;; ++i) {
    const int needInterval = isempty1f(self->currentInterval.tRange);
    int haveInterval       = !needInterval;
    intervalIterator->iterate((const int *uniform) & needInterval,
                              self->intervalIteratorState,
                              &self->currentInterval,
                              self->valueSelector->valuesMinMax,
                              (int *uniform) & haveInterval);

    if (!haveInterval) {
      // We use this to indicate that this lane has run out of intervals,
      // so that we can use an early exit on the next iterateHit call.
      self->currentInterval.tRange.lower = inf;
      return;
    }

    varying float time =
        0.f;  // TODO: should be value from API, as the default hit iterator can
              // be used with volumes that support time

    varying Hit *uniform hit = (varying Hit * uniform) _hit;
    hit->t                   = inf;
    bool foundHit =
        intersectSurfacesBisection(self->sampler,
                                   self->origin,
                                   self->direction,
                                   self->currentInterval.tRange,
                                   time,
                                   0.5f * self->currentInterval.nominalDeltaT,
                                   self->valueSelector->numValues,
                                   self->valueSelector->values,
                                   *hit);

    *result |= foundHit;

    if (foundHit) {
      self->currentInterval.tRange.lower = hit->t + hit->epsilon;
      return;
    }

    self->currentInterval.tRange.lower = inf;
  }
}

