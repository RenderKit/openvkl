// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DefaultIterator.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "../volume/Volume.h"
#include "DefaultIterator_ispc.h"

namespace openvkl {
  namespace cpu_device {

    ////////////////////////////////////////////////////////////////////////////

    template <int W>
    void DefaultIntervalIterator<W>::initializeIntervalV(
        const vintn<W> &valid,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange)
    {
      const Volume<W> &volume = context->getSampler().getVolume();
      box3f boundingBox  = volume.getBoundingBox();
      range1f valueRange = volume.getValueRange(0);

      CALL_ISPC(DefaultIntervalIterator_Initialize,
                static_cast<const int *>(valid),
                ispcStorage,
                context->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange,
                (const ispc::box3f &)boundingBox,
                (const ispc::box1f &)valueRange);
    }

    template <int W>
    void DefaultIntervalIterator<W>::iterateIntervalV(
        const vintn<W> &valid, vVKLIntervalN<W> &interval, vintn<W> &result)
    {
      CALL_ISPC(DefaultIntervalIterator_iterateInterval,
                static_cast<const int *>(valid),
                ispcStorage,
                &interval,
                static_cast<int *>(result));
    }

    template class DefaultIntervalIterator<VKL_TARGET_WIDTH>;

    __vkl_verify_max_interval_iterator_size(DefaultIntervalIterator<VKL_TARGET_WIDTH>)

  }  // namespace cpu_device
}  // namespace openvkl
