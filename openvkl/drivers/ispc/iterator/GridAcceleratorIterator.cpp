// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "GridAcceleratorIterator.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "../value_selector/ValueSelector.h"
#include "../volume/StructuredRegularVolume.h"
#include "GridAcceleratorIterator_ispc.h"

namespace openvkl {
  namespace ispc_driver {

    ///////////////////////////////////////////////////////////////////////////
    // Uniform iterator ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    constexpr int GridAcceleratorIteratorU<W>::ispcStorageSize;

    template <int W>
    GridAcceleratorIteratorU<W>::GridAcceleratorIteratorU(

        const Volume<W> *volume,
        const vvec3fn<1> &origin,
        const vvec3fn<1> &direction,
        const vrange1fn<1> &tRange,
        const ValueSelector<W> *valueSelector)
        : IteratorU<W>(volume, origin, direction, tRange, valueSelector)
    {
      static bool oneTimeChecks = false;

      if (!oneTimeChecks) {
        int ispcSize = CALL_ISPC(GridAcceleratorIteratorU_sizeOf);

        if (ispcSize != ispcStorageSize) {
          LogMessageStream(VKL_LOG_ERROR)
              << "GridAcceleratorIteratorU required ISPC object size = "
              << ispcSize << ", allocated size = " << ispcStorageSize
              << std::endl;

          throw std::runtime_error(
              "GridAcceleratorIteratorU has insufficient ISPC storage");
        }

        oneTimeChecks = true;
      }

      const StructuredRegularVolume<W> *srv =
          static_cast<const StructuredRegularVolume<W> *>(volume);

      box3f boundingBox = volume->getBoundingBox();

      CALL_ISPC(GridAcceleratorIteratorU_Initialize,
                &ispcStorage[0],
                srv->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange,
                valueSelector ? valueSelector->getISPCEquivalent() : nullptr);
    }

    template <int W>
    const Interval<1> *GridAcceleratorIteratorU<W>::getCurrentInterval() const
    {
      return reinterpret_cast<const Interval<1> *>(
          CALL_ISPC(GridAcceleratorIteratorU_getCurrentInterval,
                    (void *)&ispcStorage[0]));
      return nullptr;
    }

    template <int W>
    void GridAcceleratorIteratorU<W>::iterateInterval(vintn<1> &result)
    {
      CALL_ISPC(GridAcceleratorIteratorU_iterateInterval,
                (void *)&ispcStorage[0],
                static_cast<int *>(result));
    }

    template <int W>
    const Hit<1> *GridAcceleratorIteratorU<W>::getCurrentHit() const
    {
      return reinterpret_cast<const Hit<1> *>(CALL_ISPC(
          GridAcceleratorIteratorU_getCurrentHit, (void *)&ispcStorage[0]));
    }

    template <int W>
    void GridAcceleratorIteratorU<W>::iterateHit(vintn<1> &result)
    {
      CALL_ISPC(GridAcceleratorIteratorU_iterateHit,

                (void *)&ispcStorage[0],
                static_cast<int *>(result));
    }

    template class GridAcceleratorIteratorU<VKL_TARGET_WIDTH>;

    ///////////////////////////////////////////////////////////////////////////
    // Varying iterator ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    constexpr int GridAcceleratorIteratorV<W>::ispcStorageSize;

    template <int W>
    GridAcceleratorIteratorV<W>::GridAcceleratorIteratorV(
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
        int ispcSize = CALL_ISPC(GridAcceleratorIteratorV_sizeOf);

        if (ispcSize != ispcStorageSize) {
          LogMessageStream(VKL_LOG_ERROR)
              << "GridAcceleratorIterator required ISPC object size = "
              << ispcSize << ", allocated size = " << ispcStorageSize
              << std::endl;

          throw std::runtime_error(
              "GridAcceleratorIterator has insufficient ISPC storage");
        }

        oneTimeChecks = true;
      }

      const StructuredRegularVolume<W> *srv =
          static_cast<const StructuredRegularVolume<W> *>(volume);

      box3f boundingBox = volume->getBoundingBox();

      CALL_ISPC(GridAcceleratorIteratorV_Initialize,
                static_cast<const int *>(valid),
                &ispcStorage[0],
                srv->getISPCEquivalent(),
                (void *)&origin,
                (void *)&direction,
                (void *)&tRange,
                valueSelector ? valueSelector->getISPCEquivalent() : nullptr);
    }

    template <int W>
    const Interval<W> *GridAcceleratorIteratorV<W>::getCurrentInterval() const
    {
      return reinterpret_cast<const Interval<W> *>(
          CALL_ISPC(GridAcceleratorIteratorV_getCurrentInterval,
                    (void *)&ispcStorage[0]));
    }

    template <int W>
    void GridAcceleratorIteratorV<W>::iterateInterval(const vintn<W> &valid,
                                                      vintn<W> &result)
    {
      CALL_ISPC(GridAcceleratorIteratorV_iterateInterval,
                static_cast<const int *>(valid),
                (void *)&ispcStorage[0],
                static_cast<int *>(result));
    }

    template <int W>
    const Hit<W> *GridAcceleratorIteratorV<W>::getCurrentHit() const
    {
      return reinterpret_cast<const Hit<W> *>(CALL_ISPC(
          GridAcceleratorIteratorV_getCurrentHit, (void *)&ispcStorage[0]));
    }

    template <int W>
    void GridAcceleratorIteratorV<W>::iterateHit(const vintn<W> &valid,
                                                 vintn<W> &result)
    {
      CALL_ISPC(GridAcceleratorIteratorV_iterateHit,
                static_cast<const int *>(valid),
                (void *)&ispcStorage[0],
                static_cast<int *>(result));
    }

    template class GridAcceleratorIteratorV<VKL_TARGET_WIDTH>;

  }  // namespace ispc_driver
}  // namespace openvkl
