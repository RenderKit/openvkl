// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VdbIterator.h"
#include "../../common/export_util.h"
#include "../../iterator/Iterator.h"
#include "VdbIterator_ispc.h"
#include "VdbVolume.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    void VdbIntervalIterator<W>::initializeIntervalV(
        const vintn<W> &valid,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange)
    {
      CALL_ISPC(VdbIterator_Initialize,
                static_cast<const int *>(valid),
                ispcStorage,
                sampler->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange);
    }

    template <int W>
    void VdbIntervalIterator<W>::iterateIntervalV(const vintn<W> &valid,
                                                  vVKLIntervalN<W> &interval,
                                                  vintn<W> &result)
    {
      CALL_ISPC(VdbIterator_iterateInterval,
                static_cast<const int *>(valid),
                ispcStorage,
                &interval,
                static_cast<int *>(result));
    }

    template class VdbIntervalIterator<VKL_TARGET_WIDTH>;

    __vkl_verify_max_interval_iterator_size(VdbIntervalIterator<VKL_TARGET_WIDTH>)
    __vkl_verify_max_hit_iterator_size(VdbHitIterator<VKL_TARGET_WIDTH>)

  }  // namespace cpu_device
}  // namespace openvkl
