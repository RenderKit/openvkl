// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <type_traits>

// for unstructured interval iterator
#include "vklIterateUnstructured.h"

namespace ispc {

  template <typename IntervalIteratorType>
  struct DefaultHitIterator
  {
    HitIteratorShared hitIteratorShared;

    // The interval iterator that we use.
    IntervalIteratorType intervalIteratorState;

    // Current iteration state.
    Interval currentInterval;

    // For the last hit: t + epsilon.
    float lastHitOffsetT;
  };

  ///////////////////////////////////////////////////////////////////////////
  // Hit iterator struct definitions ////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  typedef DefaultHitIterator<UnstructuredIntervalIterator>
      UnstructuredHitIterator;

  ///////////////////////////////////////////////////////////////////////////
  // Templated hit iterator implementation //////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  template <typename HitIteratorType, auto IntervalIteratorInitFunc>
  inline void DefaultHitIterator_Init(HitIteratorType *self,
                                      const HitIteratorContext *context,
                                      const vec3f *origin,
                                      const vec3f *direction,
                                      const box1f *tRange)
  {
    self->hitIteratorShared.context = context;

    // elementary cell iteration supported for unstructured, but not other
    // volume types
    IntervalIteratorInitFunc(
        &self->intervalIteratorState,
        &self->hitIteratorShared.context->super,
        origin,
        direction,
        tRange,
        std::is_same<HitIteratorType, UnstructuredHitIterator>::value);

    self->lastHitOffsetT = neg_inf;

    // Reset to invalid.
    resetInterval(self->currentInterval);
  }

  static const float BISECT_T_TOL_ABS         = 1e-6f;
  static const float BISECT_T_TOL_FACTOR      = 0.01f;
  static const float BISECT_VALUE_TOL         = 1e-6f;
  static const int BISECT_MAX_BACKTRACK_ITERS = 10;

  // Intersect isosurfaces along the given ray using bisection method.
  template <auto SamplingFunc>
  inline float bisect(const SamplerShared *sampler,
                      const vec3f &origin,
                      const vec3f &direction,
                      float t0,
                      float sample0,
                      float t,
                      float sample,
                      const float &isovalue,
                      const float &tol,
                      float &error)
  {
    constexpr int maxIter = 10;

    int iter = 0;
    float tMid;

    while (iter < maxIter) {
      tMid = 0.5f * (t0 + t);

      error = 0.5f * (t - t0);

      if (error < tol) {
        break;
      }

      const float sampleMid = SamplingFunc(sampler, origin + tMid * direction);

      // sampling at boundaries between unstructured cells can rarely lead to
      // NaN values (indicating outside of cell) due to numerical issues; in
      // this case we know we have already bracketed an isovalue, so return the
      // nearest result *
      if (isnan(sampleMid)) {
        if (absf(isovalue - sample0) < absf(isovalue - sample)) {
          return t0;
        } else {
          return t;
        }
      }

      if (sampleMid == isovalue ||
          (isovalue - sample0) * (isovalue - sampleMid) < 0.f) {
        // crossing in (t0, tMid)
        t      = tMid;
        sample = sampleMid;
      } else if (sample == isovalue ||
                 (isovalue - sample) * (isovalue - sampleMid) < 0.f) {
        // crossing in (tMid, t)
        t0      = tMid;
        sample0 = sampleMid;
      } else {
        // should never get here
        assert(false);
        return inf;
      }

      iter++;
    }

    return tMid;
  }

  template <auto SamplingFunc>
  inline bool intersectSurfacesBisection(
      const SamplerShared *sampler,
      const UnstructuredIntervalIterator *intervalIterator,
      const Interval &interval,
      const int numValues,
      const float *values,
      VKLHit &hit)
  {
    assert(interval.tRange.lower < interval.tRange.upper);

    float t0      = interval.tRange.lower;
    float sample0 = SamplingFunc(
        sampler, intervalIterator->origin + t0 * intervalIterator->direction);

    {
      int iters = 0;

      while (iters < BISECT_MAX_BACKTRACK_ITERS && isnan(sample0) &&
             t0 < interval.tRange.upper) {
        t0 += max(1e-5f, 1e-5f * interval.nominalDeltaT);
        sample0 = SamplingFunc(
            sampler,
            intervalIterator->origin + t0 * intervalIterator->direction);
        iters++;
      }
    }

    float t;

    while (t0 < interval.tRange.upper) {
      const float h = min(interval.nominalDeltaT, interval.tRange.upper - t0);
      t             = t0 + h;
      float sample  = SamplingFunc(
          sampler, intervalIterator->origin + t * intervalIterator->direction);
      float ts = t;

      {
        int iters = 0;

        while (iters < BISECT_MAX_BACKTRACK_ITERS && isnan(sample) && ts > t0) {
          ts -= max(1e-5f, 1e-5f * interval.nominalDeltaT);
          sample = SamplingFunc(
              sampler,
              intervalIterator->origin + ts * intervalIterator->direction);
          iters++;
        }
      }

      float tHit    = inf;
      float epsilon = inf;
      float value   = inf;

      for (int i = 0; i < numValues; i++) {
        // hit at bracket entrance
        if (absf(sample0 - values[i]) < BISECT_VALUE_TOL) {
          if (t0 < tHit && t0 <= interval.tRange.upper) {
            tHit    = t0;
            value   = values[i];
            epsilon = 0.125f * interval.nominalDeltaT;
          }
        } else if (!isnan(sample0 + sample) &&
                   (values[i] - sample0) * (values[i] - sample) < 0.f) {
          // we have bracketed a crossing; bisect
          float error;
          const float tIso = bisect<SamplingFunc>(
              sampler,
              intervalIterator->origin,
              intervalIterator->direction,
              t0,
              sample0,
              ts,
              sample,
              values[i],
              min(BISECT_T_TOL_ABS, BISECT_T_TOL_FACTOR * h),
              error);

          if (tIso < tHit && tIso <= interval.tRange.upper) {
            tHit    = tIso;
            value   = values[i];
            epsilon = 0.125f * interval.nominalDeltaT;
          }
        }

        // check hit at tRange upper limit, after any bisection has occurred
        if (t == interval.tRange.upper &&
            absf(sample - values[i]) < BISECT_VALUE_TOL) {
          if (ts < tHit && ts <= interval.tRange.upper) {
            tHit    = ts;
            value   = values[i];
            epsilon = 0.125f * interval.nominalDeltaT;
          }
        }
      }

      if (tHit < inf) {
        hit.t      = tHit;
        hit.sample = value;
        hit.epsilon =
            epsilon * length(intervalIterator->direction);  // in object space
        return true;
      }

      t0      = t;
      sample0 = sample;
    }

    return false;
  }

  template <typename HitIteratorType,
            auto IntervalIteratorIterateFunc,
            auto SamplingFunc>
  inline void DefaultHitIterator_iterateHit(HitIteratorType *self,
                                            VKLHit *hit,
                                            int *result)
  {
    *result = false;

    // The selector prunes everything - don't iterate at all.
    if (self->hitIteratorShared.context->numValues == 0)
      return;

    if (self->currentInterval.tRange.lower == inf)  // Ray has finished already.
      return;

    // Generate intervals using the interval iterator until we either run out
    // or find an isosurface hit.

    // We enable elementary cell iteration for more robust bracketing intervals,
    // if the underlying interval iterator supports it.
    constexpr bool elementaryCellIteration =
        std::is_same<HitIteratorType, UnstructuredHitIterator>::value;

    while (true) {
      const int needInterval = isempty1f(self->currentInterval.tRange);
      int haveInterval       = !needInterval;

      if (needInterval) {
        IntervalIteratorIterateFunc(
            &self->intervalIteratorState,
            &self->currentInterval,
            self->hitIteratorShared.context->super.super.valueRanges,
            elementaryCellIteration,
            &haveInterval);
      }

      // We've found a new interval, but it's possible the last hit may have
      // moved us past the beginning of this new interval. This can occur with
      // hits on interval boundaries, for example. We also need to check if this
      // nudges us out of the current interval.
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

      hit->t        = inf;
      bool foundHit = intersectSurfacesBisection<SamplingFunc>(
          self->hitIteratorShared.context->super.super.sampler,
          &self->intervalIteratorState,
          self->currentInterval,
          self->hitIteratorShared.context->numValues,
          self->hitIteratorShared.context->values,
          *hit);

      *result = foundHit;

      if (foundHit) {
        self->currentInterval.tRange.lower = hit->t + hit->epsilon;
        self->lastHitOffsetT               = hit->t + hit->epsilon;
        return;
      }

      self->currentInterval.tRange.lower = inf;
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // Volume-specific hit iterator API entry points //////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  constexpr auto UnstructuredHitIterator_Init =
      &DefaultHitIterator_Init<UnstructuredHitIterator,
                               UnstructuredIntervalIterator_Init>;

  constexpr auto UnstructuredHitIterator_iterateHit =
      &DefaultHitIterator_iterateHit<
          UnstructuredHitIterator,
          UnstructuredIntervalIterator_iterateInternal,
          UnstructuredVolume_sample>;

}  // namespace ispc
