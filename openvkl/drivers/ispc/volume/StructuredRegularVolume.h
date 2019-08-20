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
#include "../samples_mask/GridAcceleratorSamplesMask.h"
#include "SharedStructuredVolume_ispc.h"
#include "StructuredVolume.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct StructuredRegularVolume : public StructuredVolume<W>
    {
      ~StructuredRegularVolume();

      void commit() override;

      void initIntervalIteratorV(vVKLIntervalIteratorN<W> &iterator,
                                 const vvec3fn<W> &origin,
                                 const vvec3fn<W> &direction,
                                 const vrange1fn<W> &tRange,
                                 const SamplesMask *samplesMask) override
      {
        iterator = toVKLIntervalIterator<W>(GridAcceleratorIterator<W>(
            this, origin, direction, tRange, samplesMask));
      }

      void iterateIntervalV(const int *valid,
                            vVKLIntervalIteratorN<W> &iterator,
                            vVKLIntervalN<W> &interval,
                            vintn<W> &result) override
      {
        GridAcceleratorIterator<W> *ri =
            fromVKLIntervalIterator<GridAcceleratorIterator<W>>(&iterator);

        ri->iterateInterval(valid, result);

        interval = *reinterpret_cast<const vVKLIntervalN<W> *>(
            ri->getCurrentInterval());
      }

      void initHitIteratorV(vVKLHitIteratorN<W> &iterator,
                            const vvec3fn<W> &origin,
                            const vvec3fn<W> &direction,
                            const vrange1fn<W> &tRange,
                            const SamplesMask *samplesMask) override
      {
        iterator = toVKLHitIterator<W>(GridAcceleratorIterator<W>(
            this, origin, direction, tRange, samplesMask));
      }

      void iterateHitV(const int *valid,
                       vVKLHitIteratorN<W> &iterator,
                       vVKLHitN<W> &hit,
                       vintn<W> &result) override
      {
        GridAcceleratorIterator<W> *ri =
            fromVKLHitIterator<GridAcceleratorIterator<W>>(&iterator);

        ri->iterateHit(valid, result);

        hit = *reinterpret_cast<const vVKLHitN<W> *>(ri->getCurrentHit());
      }

      SamplesMask *newSamplesMask() override
      {
        return new GridAcceleratorSamplesMask<W>(this);
      }

      void computeSampleV(const int *valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples) const override
      {
        ispc::SharedStructuredVolume_sample_export(
            valid, ispcEquivalent, &objectCoordinates, &samples);
      }

      // TODO
      vec3f computeGradient(const vec3f &objectCoordinates) const override
      {
        throw std::runtime_error("computeGradient() not implemented");
      }

      box3f getBoundingBox() const override
      {
        ispc::box3f bb =
            ispc::SharedStructuredVolume_getBoundingBox(ispcEquivalent);

        return box3f(vec3f(bb.lower.x, bb.lower.y, bb.lower.z),
                     vec3f(bb.upper.x, bb.upper.y, bb.upper.z));
      }

      void *getISPCEquivalent() const
      {
        return ispcEquivalent;
      }

     protected:
      void buildAccelerator();

      Data *voxelData{nullptr};
      void *ispcEquivalent{nullptr};
    };

  }  // namespace ispc_driver
}  // namespace openvkl
