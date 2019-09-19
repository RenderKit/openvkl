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

#pragma once

#include "../common/Data.h"
#include "../iterator/GridAcceleratorIterator.h"
#include "SharedStructuredVolume_ispc.h"
#include "StructuredVolume.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct StructuredRegularVolume : public StructuredVolume<W>
    {
      ~StructuredRegularVolume();

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

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples) const override;

      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients) const override;

      box3f getBoundingBox() const override;

     protected:
      void buildAccelerator();

      Data *voxelData{nullptr};
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
      iterator = toVKLIntervalIterator<W>(GridAcceleratorIterator<W>(
          valid, this, origin, direction, tRange, valueSelector));
    }

    template <int W>
    inline void StructuredRegularVolume<W>::iterateIntervalV(
        const vintn<W> &valid,
        vVKLIntervalIteratorN<W> &iterator,
        vVKLIntervalN<W> &interval,
        vintn<W> &result)
    {
      GridAcceleratorIterator<W> *ri =
          fromVKLIntervalIterator<GridAcceleratorIterator<W>>(&iterator);

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
      iterator = toVKLHitIterator<W>(GridAcceleratorIterator<W>(
          valid, this, origin, direction, tRange, valueSelector));
    }

    template <int W>
    inline void StructuredRegularVolume<W>::iterateHitV(
        const vintn<W> &valid,
        vVKLHitIteratorN<W> &iterator,
        vVKLHitN<W> &hit,
        vintn<W> &result)
    {
      GridAcceleratorIterator<W> *ri =
          fromVKLHitIterator<GridAcceleratorIterator<W>>(&iterator);

      ri->iterateHit(valid, result);

      hit = *reinterpret_cast<const vVKLHitN<W> *>(ri->getCurrentHit());
    }

    template <int W>
    inline void StructuredRegularVolume<W>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples) const
    {
      ispc::SharedStructuredVolume_sample_export((const int *)&valid,
                                                 this->ispcEquivalent,
                                                 &objectCoordinates,
                                                 &samples);
    }

    template <int W>
    inline void StructuredRegularVolume<W>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients) const
    {
      ispc::SharedStructuredVolume_gradient_export((const int *)&valid,
                                                   this->ispcEquivalent,
                                                   &objectCoordinates,
                                                   &gradients);
    }

    template <int W>
    inline box3f StructuredRegularVolume<W>::getBoundingBox() const
    {
      ispc::box3f bb =
          ispc::SharedStructuredVolume_getBoundingBox(this->ispcEquivalent);

      return box3f(vec3f(bb.lower.x, bb.lower.y, bb.lower.z),
                   vec3f(bb.upper.x, bb.upper.y, bb.upper.z));
    }

  }  // namespace ispc_driver
}  // namespace openvkl
