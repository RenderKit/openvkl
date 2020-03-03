// ======================================================================== //
// Copyright 2019-2020 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "GridAcceleratorIterator.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "../value_selector/ValueSelector.h"
#include "../volume/StructuredRegularVolume.h"
#include "GridAcceleratorIterator_ispc.h"

namespace openvkl {
  namespace ispc_driver {

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
        int ispcSize = CALL_ISPC(GridAcceleratorIterator_sizeOf);

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

      CALL_ISPC(GridAcceleratorIterator_Initialize,
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
      return reinterpret_cast<const Interval<W> *>(CALL_ISPC(
          GridAcceleratorIterator_getCurrentInterval, (void *)&ispcStorage[0]));
    }

    template <int W>
    void GridAcceleratorIteratorV<W>::iterateInterval(const vintn<W> &valid,
                                                      vintn<W> &result)
    {
      CALL_ISPC(GridAcceleratorIterator_iterateInterval,
                static_cast<const int *>(valid),
                (void *)&ispcStorage[0],
                static_cast<int *>(result));
    }

    template <int W>
    const Hit<W> *GridAcceleratorIteratorV<W>::getCurrentHit() const
    {
      return reinterpret_cast<const Hit<W> *>(CALL_ISPC(
          GridAcceleratorIterator_getCurrentHit, (void *)&ispcStorage[0]));
    }

    template <int W>
    void GridAcceleratorIteratorV<W>::iterateHit(const vintn<W> &valid,
                                                 vintn<W> &result)
    {
      CALL_ISPC(GridAcceleratorIterator_iterateHit,
                static_cast<const int *>(valid),
                (void *)&ispcStorage[0],
                static_cast<int *>(result));
    }

    template class GridAcceleratorIteratorV<VKL_TARGET_WIDTH>;

  }  // namespace ispc_driver
}  // namespace openvkl
