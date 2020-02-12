// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#include "UnstructuredIterator.h"
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
        : Iterator<W>(valid, volume, origin, direction, tRange, valueSelector)
    {
      static bool oneTimeChecks = false;

      if (!oneTimeChecks) {
        int ispcSize = ispc::UnstructuredIterator_sizeOf();

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

      ispc::UnstructuredIterator_Initialize(
          (const int *)&valid,
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
      return reinterpret_cast<const Interval<W> *>(
          ispc::UnstructuredIterator_getCurrentInterval(
              (void *)&ispcStorage[0]));
    }

    template <int W>
    void UnstructuredIterator<W>::iterateInterval(const vintn<W> &valid,
                                                  vintn<W> &result)
    {
      ispc::UnstructuredIterator_iterateInterval(
          (const int *)&valid, (void *)&ispcStorage[0], (int *)&result);
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

#if TARGET_WIDTH_ENABLED_4
    template class UnstructuredIterator<4>;
#endif

#if TARGET_WIDTH_ENABLED_8
    template class UnstructuredIterator<8>;
#endif

#if TARGET_WIDTH_ENABLED_16
    template class UnstructuredIterator<16>;
#endif

  }  // namespace ispc_driver
}  // namespace openvkl
