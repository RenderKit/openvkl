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

namespace volley {
  namespace ispc_driver {

    template <int W>
    GridAcceleratorRayIterator<W>::GridAcceleratorRayIterator(
        const Volume *volume,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const SamplesMask *samplesMask)
        : RayIterator<W>(volume, origin, direction, tRange, samplesMask)
    {
      const SharedStructuredVolume *ssv =
          static_cast<const SharedStructuredVolume *>(volume);

      const GridAcceleratorSamplesMask *gasm =
          static_cast<const GridAcceleratorSamplesMask *>(samplesMask);

      box3f boundingBox = volume->getBoundingBox();

      ispcEquivalent = ispc::GridAcceleratorRayIterator_Constructor(
          ssv->getISPCEquivalent(),
          (const ispc::box3f &)boundingBox,
          (void *)&origin,
          (void *)&direction,
          (void *)&tRange,
          gasm ? gasm->getISPCEquivalent() : nullptr);
    }

    template <int W>
    GridAcceleratorRayIterator<W>::~GridAcceleratorRayIterator()
    {
      ispc::GridAcceleratorRayIterator_Destructor(ispcEquivalent);
    }

    template <int W>
    const RayInterval<W> *GridAcceleratorRayIterator<W>::getCurrentRayInterval()
        const
    {
      return reinterpret_cast<const RayInterval<W> *>(
          ispc::GridAcceleratorRayIterator_getCurrentRayInterval(
              ispcEquivalent));
    }

    template <int W>
    void GridAcceleratorRayIterator<W>::iterateInterval(const int *valid,
                                                        vintn<8> &result)
    {
      ispc::GridAcceleratorRayIterator_iterateInterval(
          valid, ispcEquivalent, (int *)&result);
    }

    template class GridAcceleratorRayIterator<1>;
    template class GridAcceleratorRayIterator<8>;

  }  // namespace ispc_driver
}  // namespace volley
