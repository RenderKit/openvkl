// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "UnstructuredIterator.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "../value_selector/ValueSelector.h"
#include "../volume/UnstructuredVolume.h"
#include "../volume/Volume.h"
#include "UnstructuredIterator_ispc.h"
#include "rkcommon/math/box.h"

namespace openvkl {
  namespace cpu_device {

    ///////////////////////////////////////////////////////////////////////////
    // Iterator.
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    void UnstructuredIntervalIterator<W>::initializeIntervalV(
        const vintn<W> &valid,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      CALL_ISPC(UnstructuredIterator_Initialize,
                static_cast<const int *>(valid),
                ispcStorage,
                sampler->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange,
                valueSelector ? valueSelector->getISPCEquivalent() : nullptr);
    }

    template <int W>
    void UnstructuredIntervalIterator<W>::iterateIntervalV(
        const vintn<W> &valid, vVKLIntervalN<W> &interval, vintn<W> &result)
    {
      CALL_ISPC(UnstructuredIterator_iterateInterval,
                static_cast<const int *>(valid),
                ispcStorage,
                &interval,
                static_cast<int *>(result));
    }

    template class UnstructuredIntervalIterator<VKL_TARGET_WIDTH>;

    __vkl_verify_max_interval_iterator_size(UnstructuredIntervalIterator<VKL_TARGET_WIDTH>)
    __vkl_verify_max_hit_iterator_size(UnstructuredHitIterator<VKL_TARGET_WIDTH>)

  }  // namespace cpu_device
}  // namespace openvkl
