// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/math.h"
#include "Iterator.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct UnstructuredVolume;

    struct Node;

    template <int W>
    struct UnstructuredIterator : public IteratorV<W>
    {
      UnstructuredIterator(const vintn<W> &valid,
                           const Volume<W> *volume,
                           const vvec3fn<W> &origin,
                           const vvec3fn<W> &direction,
                           const vrange1fn<W> &tRange,
                           const ValueSelector<W> *valueSelector);

      const Interval<W> *getCurrentInterval() const override;
      void iterateInterval(const vintn<W> &valid, vintn<W> &result) override;

      const Hit<W> *getCurrentHit() const override;
      void iterateHit(const vintn<W> &valid, vintn<W> &result) override;

      // required size of ISPC-side object for width
      static constexpr int ispcStorageSize = 76 * W;

     protected:
      alignas(simd_alignment_for_width(W)) char ispcStorage[ispcStorageSize];
    };

  }  // namespace ispc_driver
}  // namespace openvkl
