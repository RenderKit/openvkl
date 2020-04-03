// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/simd.h"
#include "../value_selector/ValueSelector.h"
#include "openvkl/openvkl.h"

using namespace ospcommon;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct Volume;

    template <int W>
    struct Interval
    {
      vrange1fn<W> tRange;
      vrange1fn<W> valueRange;
      vfloatn<W> nominalDeltaT;
    };

    template <int W>
    struct Hit
    {
      vfloatn<W> t;
      vfloatn<W> sample;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Conversion to / from public types //////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // constructs a given iterator type in the internal state of a public
    // iterator object; does NOT populate the volume member of the public
    // iterator object
    template <typename T, int W, typename... Args>
    inline void initVKLIntervalIterator(vVKLIntervalIteratorN<W> &iterator,
                                        const Args &... args)
    {
      static_assert(
          iterator_internal_state_size_for_width(W) >= sizeof(T),
          "iterator internal state size must be >= source object size");
      T *t = new (&iterator.internalState) T(args...);
    }

    template <typename T, int W>
    inline T *fromVKLIntervalIterator(vVKLIntervalIteratorN<W> *x)
    {
      static_assert(
          alignof(T) <= alignof(vVKLIntervalIteratorN<W>),
          "alignment of destination type must be == alignment of source type");
      static_assert(
          sizeof(T) <= iterator_internal_state_size_for_width(W),
          "fromVKLIntervalIterator destination object size must be <= "
          "iterator internal state size");
      return reinterpret_cast<T *>(&x->internalState[0]);
    }

    // constructs a given iterator type in the internal state of a public
    // iterator object; does NOT populate the volume member of the public
    // iterator object
    template <typename T, int W, typename... Args>
    inline void initVKLHitIterator(vVKLHitIteratorN<W> &iterator,
                                   const Args &... args)
    {
      static_assert(
          iterator_internal_state_size_for_width(W) >= sizeof(T),
          "iterator internal state size must be >= source object size");
      T *t = new (&iterator.internalState) T(args...);
    }

    template <typename T, int W>
    inline T *fromVKLHitIterator(vVKLHitIteratorN<W> *x)
    {
      static_assert(
          alignof(T) <= alignof(vVKLHitIteratorN<W>),
          "alignment of destination type must be == alignment of source type");
      static_assert(sizeof(T) <= iterator_internal_state_size_for_width(W),
                    "fromVKLHitIterator destination object size must be <= "
                    "iterator internal state size");
      return reinterpret_cast<T *>(&x->internalState[0]);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Uniform iterator ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // a general iterator that handles both interval and hit iteration
    template <int W>
    struct IteratorU
    {
      IteratorU() = default;

      IteratorU(const Volume<W> *volume,
                const vvec3fn<1> &origin,
                const vvec3fn<1> &direction,
                const vrange1fn<1> &tRange,
                const ValueSelector<W> *valueSelector);

      virtual ~IteratorU() = default;

      virtual const Interval<1> *getCurrentInterval() const = 0;
      virtual void iterateInterval(vintn<1> &result)        = 0;

      virtual const Hit<1> *getCurrentHit() const = 0;
      virtual void iterateHit(vintn<1> &result)   = 0;

      const Volume<W> *volume;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline IteratorU<W>::IteratorU(const Volume<W> *volume,
                                   const vvec3fn<1> &origin,
                                   const vvec3fn<1> &direction,
                                   const vrange1fn<1> &tRange,
                                   const ValueSelector<W> *valueSelector)
        : volume(volume)
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    // Varying iterator ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // a general iterator that handles both interval and hit iteration
    template <int W>
    struct IteratorV
    {
      IteratorV() = default;

      IteratorV(const vintn<W> &valid,
                const Volume<W> *volume,
                const vvec3fn<W> &origin,
                const vvec3fn<W> &direction,
                const vrange1fn<W> &tRange,
                const ValueSelector<W> *valueSelector);

      virtual ~IteratorV() = default;

      virtual const Interval<W> *getCurrentInterval() const                 = 0;
      virtual void iterateInterval(const vintn<W> &valid, vintn<W> &result) = 0;

      virtual const Hit<W> *getCurrentHit() const                      = 0;
      virtual void iterateHit(const vintn<W> &valid, vintn<W> &result) = 0;

      const Volume<W> *volume;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline IteratorV<W>::IteratorV(const vintn<W> &valid,
                                   const Volume<W> *volume,
                                   const vvec3fn<W> &origin,
                                   const vvec3fn<W> &direction,
                                   const vrange1fn<W> &tRange,
                                   const ValueSelector<W> *valueSelector)
        : volume(volume)
    {
    }

  }  // namespace ispc_driver
}  // namespace openvkl
