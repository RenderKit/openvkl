// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "StructuredVolume.h"
#include "../iterator/DefaultIterator.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    using StructuredSphericalIntervalIteratorFactory =
        ConcreteIteratorFactory<W,
                                IntervalIterator,
                                DefaultIntervalIterator>;

    template <int W>
    using StructuredSphericalHitIterator = DefaultHitIterator<W, DefaultIntervalIterator<W>>;

    template <int W>
    using StructuredSphericalHitIteratorFactory =
        ConcreteIteratorFactory<W, HitIterator, StructuredSphericalHitIterator>;

    template <int W>
    struct StructuredSphericalVolume : public StructuredVolume<W>
    {
      std::string toString() const override;

      void commit() override;

      const IteratorFactory<W, IntervalIterator> &getIntervalIteratorFactory()
          const override final
      {
        return intervalIteratorFactory;
      }

      const IteratorFactory<W, HitIterator> &getHitIteratorFactory()
          const override final
      {
        return hitIteratorFactory;
      }

      // note, although we construct the GridAccelerator for all structured
      // volumes (including this one), we only use it here for computing
      // value range. we don't yet use it for iterators since the nextCell()
      // implementation is only correct for structured regular volumes.

      StructuredSphericalIntervalIteratorFactory<W> intervalIteratorFactory;
      StructuredSphericalHitIteratorFactory<W> hitIteratorFactory;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline std::string StructuredSphericalVolume<W>::toString() const
    {
      return "openvkl::StructuredSphericalVolume";
    }

  }  // namespace ispc_driver
}  // namespace openvkl
