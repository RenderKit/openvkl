// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "UnstructuredIterator.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "../value_selector/ValueSelector.h"
#include "../volume/UnstructuredVolume.h"
#include "../volume/Volume.h"
#include "UnstructuredIterator_ispc.h"
#include "ospcommon/math/box.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    constexpr int UnstructuredIterator<W>::ispcStorageSize;

    template <int W>
    UnstructuredIterator<W>::UnstructuredIterator(
        const vintn<W> &valid,
        const Volume<W> *volume,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
        : IteratorV<W>(valid, volume, origin, direction, tRange, valueSelector)
    {
      static bool oneTimeChecks = false;

      if (!oneTimeChecks) {
        int ispcSize = CALL_ISPC(UnstructuredIterator_sizeOf);

        if (ispcSize > ispcStorageSize) {
          LogMessageStream(VKL_LOG_ERROR)
              << "Unstructured Iterator required ISPC object size = "
              << ispcSize << ", allocated size = " << ispcStorageSize
              << std::endl;

          throw std::runtime_error(
              "Unstructured Iterator has insufficient ISPC storage");
        }

        oneTimeChecks = true;
      }

      CALL_ISPC(UnstructuredIterator_Initialize,
                static_cast<const int *>(valid),
                &ispcStorage[0],
                volume->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange,
                valueSelector ? valueSelector->getISPCEquivalent() : nullptr);
    }

    template <int W>
    const Interval<W> *UnstructuredIterator<W>::getCurrentInterval() const
    {
      return reinterpret_cast<const Interval<W> *>(CALL_ISPC(
          UnstructuredIterator_getCurrentInterval, (void *)&ispcStorage[0]));
    }

    template <int W>
    void UnstructuredIterator<W>::iterateInterval(const vintn<W> &valid,
                                                  vintn<W> &result)
    {
      CALL_ISPC(UnstructuredIterator_iterateInterval,
                static_cast<const int *>(valid),
                (void *)&ispcStorage[0],
                static_cast<int *>(result));
    }

    template <int W>
    const Hit<W> *UnstructuredIterator<W>::getCurrentHit() const
    {
      throw std::runtime_error(
          "UnstructuredIterator::getCurrentHit not implemented");
      return nullptr;
    }

    template <int W>
    void UnstructuredIterator<W>::iterateHit(const vintn<W> &valid,
                                             vintn<W> &result)
    {
      throw std::runtime_error(
          "UnstructuredIterator::iterateHit not implemented");
      return;
    }

    template class UnstructuredIterator<VKL_TARGET_WIDTH>;

  }  // namespace ispc_driver
}  // namespace openvkl
