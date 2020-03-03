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

#include "../iterator/GridAcceleratorIterator.h"
#include "StructuredVolume.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct StructuredRegularVolume : public StructuredVolume<W>
    {
      void commit() override;

      void initIntervalIteratorV(
          const vintn<W> &valid,
          vVKLIntervalIteratorN<W> &iterator,
          const vvec3fn<W> &origin,
          const vvec3fn<W> &direction,
          const vrange1fn<W> &tRange,
          const ValueSelector<W> *valueSelector) override;

      void iterateIntervalV(const vintn<W> &valid,
                            vVKLIntervalIteratorN<W> &iterator,
                            vVKLIntervalN<W> &interval,
                            vintn<W> &result) override;

      void initHitIteratorV(const vintn<W> &valid,
                            vVKLHitIteratorN<W> &iterator,
                            const vvec3fn<W> &origin,
                            const vvec3fn<W> &direction,
                            const vrange1fn<W> &tRange,
                            const ValueSelector<W> *valueSelector) override;

      void iterateHitV(const vintn<W> &valid,
                       vVKLHitIteratorN<W> &iterator,
                       vVKLHitN<W> &hit,
                       vintn<W> &result) override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline void StructuredRegularVolume<W>::initIntervalIteratorV(
        const vintn<W> &valid,
        vVKLIntervalIteratorN<W> &iterator,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      iterator = toVKLIntervalIterator<W>(GridAcceleratorIteratorV<W>(
          valid, this, origin, direction, tRange, valueSelector));
    }

    template <int W>
    inline void StructuredRegularVolume<W>::iterateIntervalV(
        const vintn<W> &valid,
        vVKLIntervalIteratorN<W> &iterator,
        vVKLIntervalN<W> &interval,
        vintn<W> &result)
    {
      GridAcceleratorIteratorV<W> *ri =
          fromVKLIntervalIterator<GridAcceleratorIteratorV<W>>(&iterator);

      ri->iterateInterval(valid, result);

      interval =
          *reinterpret_cast<const vVKLIntervalN<W> *>(ri->getCurrentInterval());
    }

    template <int W>
    inline void StructuredRegularVolume<W>::initHitIteratorV(
        const vintn<W> &valid,
        vVKLHitIteratorN<W> &iterator,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      iterator = toVKLHitIterator<W>(GridAcceleratorIteratorV<W>(
          valid, this, origin, direction, tRange, valueSelector));
    }

    template <int W>
    inline void StructuredRegularVolume<W>::iterateHitV(
        const vintn<W> &valid,
        vVKLHitIteratorN<W> &iterator,
        vVKLHitN<W> &hit,
        vintn<W> &result)
    {
      GridAcceleratorIteratorV<W> *ri =
          fromVKLHitIterator<GridAcceleratorIteratorV<W>>(&iterator);

      ri->iterateHit(valid, result);

      hit = *reinterpret_cast<const vVKLHitN<W> *>(ri->getCurrentHit());
    }

  }  // namespace ispc_driver
}  // namespace openvkl
