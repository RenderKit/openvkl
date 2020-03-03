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

#pragma once

#include "../../iterator/Iterator.h"
#include "VdbGrid.h"

using namespace ospcommon;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct VdbVolume;

    template <int W>
    struct VdbIteratorSize;

    /*
     * This iterator implements a hierarchical Digital Differential Analyzer.
     */
    template <int W>
    struct VdbIterator : public IteratorV<W>
    {
      VdbIterator()  = default;
      ~VdbIterator() = default;

      VdbIterator(const vintn<W> &valid,
                  const VdbVolume<W> *volume,
                  const vvec3fn<W> &origin,
                  const vvec3fn<W> &direction,
                  const vrange1fn<W> &tRange,
                  const ValueSelector<W> *valueSelector);

      const Interval<W> *getCurrentInterval() const override;
      void iterateInterval(const vintn<W> &valid, vintn<W> &result) override;

      const Hit<W> *getCurrentHit() const override;
      void iterateHit(const vintn<W> &valid, vintn<W> &result) override;

      // Required size of ISPC-side object for width W.
      // Use the vklVdbIteratorSize<W> tools to find out the correct size.
      static constexpr int ispcStorageSize = 376 * W;

     protected:
      alignas(simd_alignment_for_width(W)) char ispcStorage[ispcStorageSize];
    };

  }  // namespace ispc_driver
}  // namespace openvkl
