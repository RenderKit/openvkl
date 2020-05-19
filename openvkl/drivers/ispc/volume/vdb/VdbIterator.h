// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../iterator/Iterator.h"
#include "VdbGrid.h"

using namespace rkcommon;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct VdbVolume;

    template <int W>
    struct VdbIteratorSize;

    /*
     * This iterator implements a hierarchical Digital Differential Analyzer.
     */
    template <int W>
    struct VdbIterator : public IteratorV<W>
    {
      VdbIterator()  = default;
      ~VdbIterator() = default;

      VdbIterator(const vintn<W> &valid,
                  const VdbVolume<W> *volume,
                  const vvec3fn<W> &origin,
                  const vvec3fn<W> &direction,
                  const vrange1fn<W> &tRange,
                  const ValueSelector<W> *valueSelector);

      const Interval<W> *getCurrentInterval() const override;
      void iterateInterval(const vintn<W> &valid, vintn<W> &result) override;

      const Hit<W> *getCurrentHit() const override;
      void iterateHit(const vintn<W> &valid, vintn<W> &result) override;

      // Required size of ISPC-side object for width W.
      // Use the vklVdbIteratorSize<W> tools to find out the correct size.
      static constexpr int ispcStorageSize = 376 * W;

     protected:
      alignas(simd_alignment_for_width(W)) char ispcStorage[ispcStorageSize];
    };

  }  // namespace ispc_driver
}  // namespace openvkl
