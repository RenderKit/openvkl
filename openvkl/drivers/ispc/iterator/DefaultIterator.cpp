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

#include "DefaultIterator.h"
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
        : Iterator<W>(valid, volume, origin, direction, tRange, valueSelector)
    {
      static bool oneTimeChecks = false;

      if (!oneTimeChecks) {
        int ispcSize = ispc::DefaultIterator_sizeOf();

        if (ispcSize > ispcStorageSize) {
          std::cerr << "DefaultIterator required ISPC object size = "
                    << ispcSize << ", allocated size = " << ispcStorageSize
                    << std::endl;

          throw std::runtime_error(
              "DefaultIterator has insufficient ISPC storage");
        }
        oneTimeChecks = true;
      }

      box3f boundingBox = volume->getBoundingBox();

      range1f valueRange = volume->getValueRange();

      ispc::DefaultIterator_Initialize(
          (const int *)&valid,
          &ispcStorage[0],
          nullptr,  // volume ISPC object not yet used
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
      return reinterpret_cast<const Interval<W> *>(
          ispc::DefaultIterator_getCurrentInterval((void *)&ispcStorage[0]));
    }

    template <int W>
    void DefaultIterator<W>::iterateInterval(const vintn<W> &valid,
                                             vintn<W> &result)
    {
      ispc::DefaultIterator_iterateInterval(
          (const int *)&valid, (void *)&ispcStorage[0], (int *)&result);
    }

    template <int W>
    const Hit<W> *DefaultIterator<W>::getCurrentHit() const
    {
      return reinterpret_cast<const Hit<W> *>(
          ispc::DefaultIterator_getCurrentHit((void *)&ispcStorage[0]));
    }

    template <int W>
    void DefaultIterator<W>::iterateHit(const vintn<W> &valid, vintn<W> &result)
    {
      ispc::DefaultIterator_iterateHit(
          (const int *)&valid, (void *)&ispcStorage[0], (int *)&result);
    }

    template class DefaultIterator<4>;
    template class DefaultIterator<8>;
    template class DefaultIterator<16>;

  }  // namespace ispc_driver
}  // namespace openvkl
