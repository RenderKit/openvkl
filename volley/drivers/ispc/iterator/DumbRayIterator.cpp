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

#include "DumbRayIterator.h"
#include "../volume/Volume.h"
#include "DumbRayIterator_ispc.h"
#include "common/math.h"

namespace volley {
  namespace ispc_driver {

    template <int W>
    DumbRayIterator<W>::DumbRayIterator(const Volume *volume,
                                        const vvec3fn<W> &origin,
                                        const vvec3fn<W> &direction,
                                        const vrange1fn<W> &tRange,
                                        const SamplesMask *samplesMask)
        : RayIterator<W>(volume, origin, direction, tRange, samplesMask)
    {
      box3f boundingBox = volume->getBoundingBox();

      ispcEquivalent =
          ispc::DumbRayIterator_create((const ispc::box3f &)boundingBox,
                                       (void *)&origin,
                                       (void *)&direction,
                                       (void *)&tRange);
    }

    template <int W>
    DumbRayIterator<W>::~DumbRayIterator()
    {
      ispc::DumbRayIterator_Destructor(ispcEquivalent);
    }

    template <int W>
    const RayInterval<W> *DumbRayIterator<W>::getCurrentRayInterval() const
    {
      return reinterpret_cast<const RayInterval<W> *>(
          ispc::DumbRayIterator_getCurrentRayInterval(ispcEquivalent));
    }

    template <int W>
    void DumbRayIterator<W>::iterateInterval(const int *valid, vintn<8> &result)
    {
      ispc::DumbRayIterator_iterateInterval(
          valid, ispcEquivalent, (int *)&result);
    }

    template class DumbRayIterator<1>;
    template class DumbRayIterator<8>;

  }  // namespace ispc_driver
}  // namespace volley
