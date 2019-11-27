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

#include "ProceduralStructuredVolume.h"
#include "procedural_functions.h"

using namespace ospcommon;

namespace openvkl {
  namespace testing {

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &) = gradientNotImplemented>
    struct ProceduralStructuredSphericalVolume
        : public ProceduralStructuredVolume<VOXEL_TYPE,
                                            samplingFunction,
                                            gradientFunction>
    {
      ProceduralStructuredSphericalVolume(const vec3i &dimensions,
                                          const vec3f &gridOrigin,
                                          const vec3f &gridSpacing);

      vec3f transformLocalToObjectCoordinates(
          const vec3f &localCoordinates) override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline ProceduralStructuredSphericalVolume<VOXEL_TYPE,
                                               samplingFunction,
                                               gradientFunction>::
        ProceduralStructuredSphericalVolume(const vec3i &dimensions,
                                            const vec3f &gridOrigin,
                                            const vec3f &gridSpacing)
        : ProceduralStructuredVolume<VOXEL_TYPE,
                                     samplingFunction,
                                     gradientFunction>(
              "structured_spherical", dimensions, gridOrigin, gridSpacing)
    {
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f ProceduralStructuredSphericalVolume<VOXEL_TYPE,
                                                     samplingFunction,
                                                     gradientFunction>::
        transformLocalToObjectCoordinates(const vec3f &localCoordinates)
    {
      const float degToRad = M_PI / 180.f;

      const float r =
          this->gridOrigin.x + localCoordinates.x * this->gridSpacing.x;

      const float inclination =
          (this->gridOrigin.y + localCoordinates.y * this->gridSpacing.y) *
          degToRad;

      const float azimuth =
          (this->gridOrigin.z + localCoordinates.z * this->gridSpacing.z) *
          degToRad;

      return vec3f(r * sinf(inclination) * cosf(azimuth),
                   r * sinf(inclination) * sinf(azimuth),
                   r * cosf(inclination));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume types ////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    using RadiusProceduralVolume =
        ProceduralStructuredSphericalVolume<float, getRadiusValue>;

  }  // namespace testing
}  // namespace openvkl
