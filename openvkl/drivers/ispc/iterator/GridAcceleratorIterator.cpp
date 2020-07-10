// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "GridAcceleratorIterator.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "../value_selector/ValueSelector.h"
#include "../volume/StructuredRegularVolume.h"

namespace openvkl {
  namespace ispc_driver {

    ///////////////////////////////////////////////////////////////////////////
    // Iterator.
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    void GridAcceleratorIntervalIterator<W>::initializeIntervalV(
        const vintn<W> &valid,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      CALL_ISPC(GridAcceleratorIteratorV_Initialize,
                static_cast<const int *>(valid),
                ispcStorage,
                volume->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange,
                valueSelector ? valueSelector->getISPCEquivalent() : nullptr);
    }

    template <int W>
    void GridAcceleratorIntervalIterator<W>::iterateIntervalV(
        const vintn<W> &valid, vVKLIntervalN<W> &interval, vintn<W> &result)
    {
      CALL_ISPC(GridAcceleratorIteratorV_iterateInterval,
                static_cast<const int *>(valid),
                ispcStorage,
                &interval,
                static_cast<int *>(result));
    }

    template <int W>
    void GridAcceleratorIntervalIterator<W>::initializeIntervalU(
        const vvec3fn<1> &origin,
        const vvec3fn<1> &direction,
        const vrange1fn<1> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      CALL_ISPC(GridAcceleratorIteratorU_Initialize,
                ispcStorage,
                volume->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange,
                valueSelector ? valueSelector->getISPCEquivalent() : nullptr);
    }

    template <int W>
    void GridAcceleratorIntervalIterator<W>::iterateIntervalU(
        vVKLIntervalN<1> &interval, vintn<1> &result)
    {
      CALL_ISPC(GridAcceleratorIteratorU_iterateInterval,
                ispcStorage,
                &interval,
                static_cast<int *>(result));
    }

    template class GridAcceleratorIntervalIterator<VKL_TARGET_WIDTH>;

    __vkl_verify_max_interval_iterator_size(GridAcceleratorIntervalIterator<VKL_TARGET_WIDTH>)

    ////////////////////////////////////////////////////////////////////////////

    template <int W>
    void GridAcceleratorHitIterator<W>::initializeHitV(
        const vintn<W> &valid,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      CALL_ISPC(GridAcceleratorIteratorV_Initialize,
                static_cast<const int *>(valid),
                ispcStorage,
                volume->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange,
                valueSelector ? valueSelector->getISPCEquivalent() : nullptr);
    }

    template <int W>
    void GridAcceleratorHitIterator<W>::iterateHitV(const vintn<W> &valid,
                                                    vVKLHitN<W> &hit,
                                                    vintn<W> &result)
    {
      // pass hit to be modified directly, which ensures that non-active lane
      // data is NOT changed (previous OSPRay issue). copies would make this
      // difficult.
      CALL_ISPC(GridAcceleratorIteratorV_iterateHit,
                static_cast<const int *>(valid),
                ispcStorage,
                &hit,
                static_cast<int *>(result));
    }

    template <int W>
    void GridAcceleratorHitIterator<W>::initializeHitU(
        const vvec3fn<1> &origin,
        const vvec3fn<1> &direction,
        const vrange1fn<1> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      CALL_ISPC(GridAcceleratorIteratorU_Initialize,
                ispcStorage,
                volume->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange,
                valueSelector ? valueSelector->getISPCEquivalent() : nullptr);
    }

    template <int W>
    void GridAcceleratorHitIterator<W>::iterateHitU(vVKLHitN<1> &hit,
                                                    vintn<1> &result)
    {
      CALL_ISPC(GridAcceleratorIteratorU_iterateHit,
                ispcStorage,
                &hit,
                static_cast<int *>(result));
    }

    template class GridAcceleratorHitIterator<VKL_TARGET_WIDTH>;

    __vkl_verify_max_hit_iterator_size(GridAcceleratorHitIterator<VKL_TARGET_WIDTH>)

  }  // namespace ispc_driver
}  // namespace openvkl
