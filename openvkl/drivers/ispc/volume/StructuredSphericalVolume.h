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

#pragma once

#include "../common/Data.h"
#include "SharedStructuredVolume_ispc.h"
#include "StructuredVolume.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct StructuredSphericalVolume : public StructuredVolume<W>
    {
      ~StructuredSphericalVolume();

      void commit() override;

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples) const override;

      box3f getBoundingBox() const override;

      range1f getValueRange() const override;

     protected:
      Data *voxelData{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline void StructuredSphericalVolume<W>::computeSampleV(
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
    inline box3f StructuredSphericalVolume<W>::getBoundingBox() const
    {
      ispc::box3f bb =
          ispc::SharedStructuredVolume_getBoundingBox(this->ispcEquivalent);

      return box3f(vec3f(bb.lower.x, bb.lower.y, bb.lower.z),
                   vec3f(bb.upper.x, bb.upper.y, bb.upper.z));
    }

    template <int W>
    inline range1f StructuredSphericalVolume<W>::getValueRange() const
    {
      throw std::runtime_error("not implemented");
    }

  }  // namespace ispc_driver
}  // namespace openvkl
