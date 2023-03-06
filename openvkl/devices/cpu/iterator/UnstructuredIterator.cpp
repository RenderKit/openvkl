// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include "../common/export_util.h"
#include "../common/math.h"
#include "../volume/UnstructuredSampler.h"
#include "../volume/UnstructuredVolume.h"
#include "../volume/Volume.h"
#include "UnstructuredIterator.h"
#include "UnstructuredIterator_ispc.h"

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
        const vfloatn<W> &_times)
    {
      // We use the same iterator implementation for both unstructured and
      // particle volumes. However, only unstructured volumes support elementary
      // cell iteration.
      const bool elementaryCellIterationSupported =
          dynamic_cast<const UnstructuredSampler<W> *>(&context->getSampler());

      CALL_ISPC(UnstructuredIterator_Initialize,
                static_cast<const int *>(valid),
                ispcStorage,
                context->getSh(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange,
                elementaryCellIterationSupported);
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

    __vkl_verify_max_interval_iterator_size(
        UnstructuredIntervalIterator<VKL_TARGET_WIDTH>)
        __vkl_verify_max_hit_iterator_size(
            UnstructuredHitIterator<VKL_TARGET_WIDTH>)

  }  // namespace cpu_device
}  // namespace openvkl
