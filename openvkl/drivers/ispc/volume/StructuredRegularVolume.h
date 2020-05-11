// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../iterator/GridAcceleratorIterator.h"
#include "StructuredVolume.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct StructuredRegularVolume : public StructuredVolume<W>
    {
      std::string toString() const override;

      void commit() override;

      void initIntervalIteratorU(
          vVKLIntervalIteratorN<1> &iterator,
          const vvec3fn<1> &origin,
          const vvec3fn<1> &direction,
          const vrange1fn<1> &tRange,
          const ValueSelector<W> *valueSelector) override;

      void initIntervalIteratorV(
          const vintn<W> &valid,
          vVKLIntervalIteratorN<W> &iterator,
          const vvec3fn<W> &origin,
          const vvec3fn<W> &direction,
          const vrange1fn<W> &tRange,
          const ValueSelector<W> *valueSelector) override;

      void iterateIntervalU(vVKLIntervalIteratorN<1> &iterator,
                            vVKLIntervalN<1> &interval,
                            vintn<1> &result) override;

      void iterateIntervalV(const vintn<W> &valid,
                            vVKLIntervalIteratorN<W> &iterator,
                            vVKLIntervalN<W> &interval,
                            vintn<W> &result) override;

      void initHitIteratorU(vVKLHitIteratorN<1> &iterator,
                            const vvec3fn<1> &origin,
                            const vvec3fn<1> &direction,
                            const vrange1fn<1> &tRange,
                            const ValueSelector<W> *valueSelector) override;

      void initHitIteratorV(const vintn<W> &valid,
                            vVKLHitIteratorN<W> &iterator,
                            const vvec3fn<W> &origin,
                            const vvec3fn<W> &direction,
                            const vrange1fn<W> &tRange,
                            const ValueSelector<W> *valueSelector) override;

      void iterateHitU(vVKLHitIteratorN<1> &iterator,
                       vVKLHitN<1> &hit,
                       vintn<1> &result) override;

      void iterateHitV(const vintn<W> &valid,
                       vVKLHitIteratorN<W> &iterator,
                       vVKLHitN<W> &hit,
                       vintn<W> &result) override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline std::string StructuredRegularVolume<W>::toString() const
    {
      return "openvkl::StructuredRegularVolume";
    }

    template <int W>
    inline void StructuredRegularVolume<W>::initIntervalIteratorU(
        vVKLIntervalIteratorN<1> &iterator,
        const vvec3fn<1> &origin,
        const vvec3fn<1> &direction,
        const vrange1fn<1> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      initVKLIntervalIterator<GridAcceleratorIteratorU<W>>(
          iterator, this, origin, direction, tRange, valueSelector);
    }

    template <int W>
    inline void StructuredRegularVolume<W>::initIntervalIteratorV(
        const vintn<W> &valid,
        vVKLIntervalIteratorN<W> &iterator,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      initVKLIntervalIterator<GridAcceleratorIteratorV<W>>(
          iterator, valid, this, origin, direction, tRange, valueSelector);
    }

    template <int W>
    inline void StructuredRegularVolume<W>::iterateIntervalU(
        vVKLIntervalIteratorN<1> &iterator,
        vVKLIntervalN<1> &interval,
        vintn<1> &result)
    {
      GridAcceleratorIteratorU<W> *ri =
          fromVKLIntervalIterator<GridAcceleratorIteratorU<W>>(&iterator);

      ri->iterateInterval(result);

      interval =
          *reinterpret_cast<const vVKLIntervalN<1> *>(ri->getCurrentInterval());
    }

    template <int W>
    inline void StructuredRegularVolume<W>::iterateIntervalV(
        const vintn<W> &valid,
        vVKLIntervalIteratorN<W> &iterator,
        vVKLIntervalN<W> &interval,
        vintn<W> &result)
    {
      GridAcceleratorIteratorV<W> *ri =
          fromVKLIntervalIterator<GridAcceleratorIteratorV<W>>(&iterator);

      ri->iterateInterval(valid, result);

      interval =
          *reinterpret_cast<const vVKLIntervalN<W> *>(ri->getCurrentInterval());
    }

    template <int W>
    inline void StructuredRegularVolume<W>::initHitIteratorU(
        vVKLHitIteratorN<1> &iterator,
        const vvec3fn<1> &origin,
        const vvec3fn<1> &direction,
        const vrange1fn<1> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      initVKLHitIterator<GridAcceleratorIteratorU<W>>(
          iterator, this, origin, direction, tRange, valueSelector);
    }

    template <int W>
    inline void StructuredRegularVolume<W>::initHitIteratorV(
        const vintn<W> &valid,
        vVKLHitIteratorN<W> &iterator,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      initVKLHitIterator<GridAcceleratorIteratorV<W>>(
          iterator, valid, this, origin, direction, tRange, valueSelector);
    }

    template <int W>
    inline void StructuredRegularVolume<W>::iterateHitU(
        vVKLHitIteratorN<1> &iterator, vVKLHitN<1> &hit, vintn<1> &result)
    {
      GridAcceleratorIteratorU<W> *ri =
          fromVKLHitIterator<GridAcceleratorIteratorU<W>>(&iterator);

      ri->iterateHit(result);

      hit = *reinterpret_cast<const vVKLHitN<1> *>(ri->getCurrentHit());
    }

    template <int W>
    inline void StructuredRegularVolume<W>::iterateHitV(
        const vintn<W> &valid,
        vVKLHitIteratorN<W> &iterator,
        vVKLHitN<W> &hit,
        vintn<W> &result)
    {
      GridAcceleratorIteratorV<W> *ri =
          fromVKLHitIterator<GridAcceleratorIteratorV<W>>(&iterator);

      ri->iterateHit(valid, result);

      hit = *reinterpret_cast<const vVKLHitN<W> *>(ri->getCurrentHit());
    }

  }  // namespace ispc_driver
}  // namespace openvkl
