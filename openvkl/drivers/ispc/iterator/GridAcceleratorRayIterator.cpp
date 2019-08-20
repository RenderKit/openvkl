// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#include "GridAcceleratorRayIterator.h"
#include "../common/math.h"
#include "../samples_mask/GridAcceleratorSamplesMask.h"
#include "../volume/StructuredRegularVolume.h"
#include "GridAcceleratorRayIterator_ispc.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    constexpr int GridAcceleratorIterator<W>::ispcStorageSize;

    template <int W>
    GridAcceleratorIterator<W>::GridAcceleratorIterator(
        const Volume<W> *volume,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const SamplesMask *samplesMask)
        : Iterator<W>(volume, origin, direction, tRange, samplesMask)
    {
      static bool oneTimeChecks = false;

      if (!oneTimeChecks) {
        int ispcSize = ispc::GridAcceleratorIterator_sizeOf();

        if (ispcSize > ispcStorageSize) {
          std::cerr << "GridAcceleratorIterator required ISPC object size = "
                    << ispcSize << ", allocated size = " << ispcStorageSize
                    << std::endl;

          throw std::runtime_error(
              "GridAcceleratorIterator has insufficient ISPC storage");
        }
        oneTimeChecks = true;
      }

      const StructuredRegularVolume<W> *srv =
          static_cast<const StructuredRegularVolume<W> *>(volume);

      const GridAcceleratorSamplesMask<W> *gasm =
          static_cast<const GridAcceleratorSamplesMask<W> *>(samplesMask);

      box3f boundingBox = volume->getBoundingBox();

      ispc::GridAcceleratorIterator_Initialize(
          &ispcStorage[0],
          srv->getISPCEquivalent(),
          (void *)&origin,
          (void *)&direction,
          (void *)&tRange,
          gasm ? gasm->getISPCEquivalent() : nullptr);
    }

    template <int W>
    const Interval<W> *GridAcceleratorIterator<W>::getCurrentInterval() const
    {
      return reinterpret_cast<const Interval<W> *>(
          ispc::GridAcceleratorIterator_getCurrentInterval(
              (void *)&ispcStorage[0]));
    }

    template <int W>
    void GridAcceleratorIterator<W>::iterateInterval(const int *valid,
                                                     vintn<W> &result)
    {
      ispc::GridAcceleratorIterator_iterateInterval(
          valid, (void *)&ispcStorage[0], (int *)&result);
    }

    template <int W>
    const Hit<W> *GridAcceleratorIterator<W>::getCurrentHit() const
    {
      return reinterpret_cast<const Hit<W> *>(
          ispc::GridAcceleratorIterator_getCurrentHit((void *)&ispcStorage[0]));
    }

    template <int W>
    void GridAcceleratorIterator<W>::iterateHit(const int *valid,
                                                vintn<W> &result)
    {
      ispc::GridAcceleratorIterator_iterateHit(
          valid, (void *)&ispcStorage[0], (int *)&result);
    }

    template class GridAcceleratorIterator<4>;
    template class GridAcceleratorIterator<8>;
    template class GridAcceleratorIterator<16>;

  }  // namespace ispc_driver
}  // namespace openvkl
