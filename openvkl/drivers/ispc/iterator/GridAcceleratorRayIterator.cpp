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
#include "../samples_mask/GridAcceleratorSamplesMask.h"
#include "../volume/SharedStructuredVolume.h"
#include "GridAcceleratorRayIterator_ispc.h"
#include "common/math.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    constexpr int GridAcceleratorRayIterator<W>::ispcStorageSize;

    template <int W>
    GridAcceleratorRayIterator<W>::GridAcceleratorRayIterator(
        const Volume<W> *volume,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const SamplesMask *samplesMask)
        : RayIterator<W>(volume, origin, direction, tRange, samplesMask)
    {
      static bool oneTimeChecks = false;

      if (!oneTimeChecks) {
        int ispcSize = ispc::GridAcceleratorRayIterator_sizeOf();

        if (ispcSize > ispcStorageSize) {
          std::cerr << "GridAcceleratorRayIterator required ISPC object size = "
                    << ispcSize << ", allocated size = " << ispcStorageSize
                    << std::endl;

          throw std::runtime_error(
              "GridAcceleratorRayIterator has insufficient ISPC storage");
        }
        oneTimeChecks = true;
      }

      const SharedStructuredVolume<W> *ssv =
          static_cast<const SharedStructuredVolume<W> *>(volume);

      const GridAcceleratorSamplesMask<W> *gasm =
          static_cast<const GridAcceleratorSamplesMask<W> *>(samplesMask);

      box3f boundingBox = volume->getBoundingBox();

      ispc::GridAcceleratorRayIterator_Initialize(
          &ispcStorage[0],
          ssv->getISPCEquivalent(),
          (void *)&origin,
          (void *)&direction,
          (void *)&tRange,
          gasm ? gasm->getISPCEquivalent() : nullptr);
    }

    template <int W>
    const RayInterval<W> *GridAcceleratorRayIterator<W>::getCurrentRayInterval()
        const
    {
      return reinterpret_cast<const RayInterval<W> *>(
          ispc::GridAcceleratorRayIterator_getCurrentRayInterval(
              (void *)&ispcStorage[0]));
    }

    template <int W>
    void GridAcceleratorRayIterator<W>::iterateInterval(const int *valid,
                                                        vintn<W> &result)
    {
      ispc::GridAcceleratorRayIterator_iterateInterval(
          valid, (void *)&ispcStorage[0], (int *)&result);
    }

    template <int W>
    const SurfaceHit<W> *GridAcceleratorRayIterator<W>::getCurrentSurfaceHit()
        const
    {
      return reinterpret_cast<const SurfaceHit<W> *>(
          ispc::GridAcceleratorRayIterator_getCurrentSurfaceHit(
              (void *)&ispcStorage[0]));
    }

    template <int W>
    void GridAcceleratorRayIterator<W>::iterateSurface(const int *valid,
                                                       vintn<W> &result)
    {
      ispc::GridAcceleratorRayIterator_iterateSurface(
          valid, (void *)&ispcStorage[0], (int *)&result);
    }

    template class GridAcceleratorRayIterator<4>;
    template class GridAcceleratorRayIterator<8>;
    template class GridAcceleratorRayIterator<16>;

  }  // namespace ispc_driver
}  // namespace openvkl
