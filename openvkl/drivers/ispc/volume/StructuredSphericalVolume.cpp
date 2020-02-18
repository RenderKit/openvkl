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

#include "StructuredSphericalVolume.h"
#include "../common/export_util.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    void StructuredSphericalVolume<W>::commit()
    {
      StructuredVolume<W>::commit();

      if (!this->ispcEquivalent) {
        this->ispcEquivalent = CALL_ISPC(SharedStructuredVolume_Constructor);

        if (!this->ispcEquivalent) {
          throw std::runtime_error(
              "could not create ISPC-side object for "
              "StructuredSphericalVolume");
        }
      }

      // each object coordinate must correspond to a unique logical coordinate;
      // we therefore currently require:
      // - radius >= 0
      // - inclination in [0, 180] degrees
      // - azimuth in [0, 360] degrees
      const range1f legalInclinationRange(0.f, 180.f);
      const range1f legalAzimuthRange(0.f, 360.f);

      // we can have negative gridSpacing values, so ensure we construct the
      // ranges here correctly such that min <= max
      range1f radiusRange = empty;
      radiusRange.extend(this->gridOrigin.x);
      radiusRange.extend(this->gridOrigin.x +
                         (this->dimensions.x - 1) * this->gridSpacing.x);

      range1f inclinationRange;
      inclinationRange.extend(this->gridOrigin.y);
      inclinationRange.extend(this->gridOrigin.y +
                              (this->dimensions.y - 1) * this->gridSpacing.y);

      range1f azimuthRange;
      azimuthRange.extend(this->gridOrigin.z);
      azimuthRange.extend(this->gridOrigin.z +
                          (this->dimensions.z - 1) * this->gridSpacing.z);

      if (radiusRange.lower < 0.f) {
        throw std::runtime_error(
            "StructuredSphericalVolume radius grid values must be >= 0");
      }

      if (inclinationRange.lower < legalInclinationRange.lower ||
          inclinationRange.upper > legalInclinationRange.upper) {
        throw std::runtime_error(
            "StructuredSphericalVolume inclination grid values must be in [0, "
            "180] degrees");
      }

      if (azimuthRange.lower < legalAzimuthRange.lower ||
          azimuthRange.upper > legalAzimuthRange.upper) {
        throw std::runtime_error(
            "StructuredSphericalVolume azimuth grid values must be in [0, 360] "
            "degrees");
      }

      // pre-transform all angles to radians
      const vec3f gridToRadians(1.f, M_PI / 180.f, M_PI / 180.f);

      const vec3f gridOriginRadians  = this->gridOrigin * gridToRadians;
      const vec3f gridSpacingRadians = this->gridSpacing * gridToRadians;

      bool success = CALL_ISPC(SharedStructuredVolume_set,
                               this->ispcEquivalent,
                               this->voxelData->data,
                               this->voxelData->dataType,
                               (const ispc::vec3i &)this->dimensions,
                               ispc::structured_spherical,
                               (const ispc::vec3f &)gridOriginRadians,
                               (const ispc::vec3f &)gridSpacingRadians);

      if (!success) {
        CALL_ISPC(SharedStructuredVolume_Destructor, this->ispcEquivalent);
        this->ispcEquivalent = nullptr;

        throw std::runtime_error("failed to commit StructuredSphericalVolume");
      }

      // must be last
      this->buildAccelerator();
    }

    VKL_REGISTER_VOLUME(StructuredSphericalVolume<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_structured_spherical_,
                                VKL_TARGET_WIDTH))

  }  // namespace ispc_driver
}  // namespace openvkl
