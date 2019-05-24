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

#include "../iterator/GridAcceleratorRayIterator.h"
#include "../samples_mask/GridAcceleratorSamplesMask.h"
#include "SharedStructuredVolume_ispc.h"
#include "StructuredVolume.h"
#include "common/Data.h"

namespace volley {

  namespace ispc_driver {

    template <int W>
    struct SharedStructuredVolume : public StructuredVolume<W>
    {
      ~SharedStructuredVolume();

      void commit() override;

      RayIterator<W> *newRayIteratorV(const vvec3fn<W> &origin,
                                      const vvec3fn<W> &direction,
                                      const vrange1fn<W> &tRange,
                                      const SamplesMask *samplesMask) override
      {
        return new GridAcceleratorRayIterator<W>(
            this, origin, direction, tRange, samplesMask);
      }

      SamplesMask *newSamplesMask() override
      {
        return new GridAcceleratorSamplesMask<W>(this);
      }

      float computeSample(const vec3f &objectCoordinates) const override
      {
        float sample;
        ispc::SharedStructuredVolume_sample_scalar_export(
            ispcEquivalent, (ispc::vec3f *)&objectCoordinates, &sample);
        return sample;
      }

      void computeSampleV(const int *valid,
                          const void *objectCoordinates,
                          void *samples) const override
      {
        ispc::SharedStructuredVolume_sample_export(
            valid, ispcEquivalent, objectCoordinates, samples);
      }

      // TODO
      vec3f computeGradient(const vec3f &objectCoordinates) const override
      {
        throw std::runtime_error("computeGradient() not implemented");
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
}  // namespace volley
