// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DefaultIterator.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "../value_selector/ValueSelector.h"
#include "../volume/Volume.h"
#include "DefaultIterator_ispc.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    constexpr int DefaultIterator<W>::ispcStorageSize;

    template <int W>
    DefaultIterator<W>::DefaultIterator(const vintn<W> &valid,
                                        const Volume<W> *volume,
                                        const vvec3fn<W> &origin,
                                        const vvec3fn<W> &direction,
                                        const vrange1fn<W> &tRange,
                                        const ValueSelector<W> *valueSelector)
        : IteratorV<W>(valid, volume, origin, direction, tRange, valueSelector)
    {
      static bool oneTimeChecks = false;

      if (!oneTimeChecks) {
        LogMessageStream(VKL_LOG_WARNING)
            << "using DefaultIterator for volume which has no native iterator "
               "implementation; performance may not be optimal"
            << std::endl;

        int ispcSize = CALL_ISPC(DefaultIterator_sizeOf);

        if (ispcSize > ispcStorageSize) {
          LogMessageStream(VKL_LOG_ERROR)
              << "DefaultIterator required ISPC object size = " << ispcSize
              << ", allocated size = " << ispcStorageSize << std::endl;

          throw std::runtime_error(
              "DefaultIterator has insufficient ISPC storage");
        }

        oneTimeChecks = true;
      }

      box3f boundingBox = volume->getBoundingBox();

      range1f valueRange = volume->getValueRange();

      CALL_ISPC(DefaultIterator_Initialize,
                static_cast<const int *>(valid),
                &ispcStorage[0],
                volume->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange,
                valueSelector ? valueSelector->getISPCEquivalent() : nullptr,
                (const ispc::box3f &)boundingBox,
                (const ispc::box1f &)valueRange);
    }

    template <int W>
    const Interval<W> *DefaultIterator<W>::getCurrentInterval() const
    {
      return reinterpret_cast<const Interval<W> *>(CALL_ISPC(
          DefaultIterator_getCurrentInterval, (void *)&ispcStorage[0]));
    }

    template <int W>
    void DefaultIterator<W>::iterateInterval(const vintn<W> &valid,
                                             vintn<W> &result)
    {
      CALL_ISPC(DefaultIterator_iterateInterval,
                static_cast<const int *>(valid),
                (void *)&ispcStorage[0],
                static_cast<int *>(result));
    }

    template <int W>
    const Hit<W> *DefaultIterator<W>::getCurrentHit() const
    {
      return reinterpret_cast<const Hit<W> *>(
          CALL_ISPC(DefaultIterator_getCurrentHit, (void *)&ispcStorage[0]));
    }

    template <int W>
    void DefaultIterator<W>::iterateHit(const vintn<W> &valid, vintn<W> &result)
    {
      CALL_ISPC(DefaultIterator_iterateHit,
                static_cast<const int *>(valid),
                (void *)&ispcStorage[0],
                static_cast<int *>(result));
    }

    template class DefaultIterator<VKL_TARGET_WIDTH>;

  }  // namespace ispc_driver
}  // namespace openvkl
