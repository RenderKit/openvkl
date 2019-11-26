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

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    StructuredSphericalVolume<W>::~StructuredSphericalVolume()
    {
      if (this->ispcEquivalent) {
        ispc::SharedStructuredVolume_Destructor(this->ispcEquivalent);
      }
    }

    template <int W>
    void StructuredSphericalVolume<W>::commit()
    {
      StructuredVolume<W>::commit();

      voxelData = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "voxelData", nullptr);

      if (!voxelData) {
        throw std::runtime_error("no voxelData set on volume");
      }

      if (voxelData->size() != longProduct(this->dimensions)) {
        throw std::runtime_error(
            "incorrect voxelData size for provided volume dimensions");
      }

      if (!this->ispcEquivalent) {
        this->ispcEquivalent = ispc::SharedStructuredVolume_Constructor();

        if (!this->ispcEquivalent) {
          throw std::runtime_error(
              "could not create ISPC-side object for "
              "StructuredSphericalVolume");
        }
      }

      // each object coordinate must correspond to a unique logical coordinate;
      // we therefore currently require:
      // - inclination in [0, 180] degrees
      // - azimuth in [0, 360] degrees
      const range1f legalInclinationRange(0.f, 180.f);
      const range1f legalAzimuthRange(0.f, 360.f);

      range1f inclinationRange(
          this->gridOrigin.y,
          this->gridOrigin.y + (this->dimensions.y - 1) * this->gridSpacing.y);

      range1f azimuthRange(
          this->gridOrigin.z,
          this->gridOrigin.z + (this->dimensions.z - 1) * this->gridSpacing.z);

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

      bool success = ispc::SharedStructuredVolume_set(
          this->ispcEquivalent,
          voxelData->data,
          voxelData->dataType,
          (const ispc::vec3i &)this->dimensions,
          ispc::structured_spherical,
          (const ispc::vec3f &)this->gridOrigin,
          (const ispc::vec3f &)this->gridSpacing);

      if (!success) {
        ispc::SharedStructuredVolume_Destructor(this->ispcEquivalent);
        this->ispcEquivalent = nullptr;

        throw std::runtime_error("failed to commit StructuredSphericalVolume");
      }
    }

    VKL_REGISTER_VOLUME(StructuredSphericalVolume<4>, structured_spherical_4)
    VKL_REGISTER_VOLUME(StructuredSphericalVolume<8>, structured_spherical_8)
    VKL_REGISTER_VOLUME(StructuredSphericalVolume<16>, structured_spherical_16)

  }  // namespace ispc_driver
}  // namespace openvkl
