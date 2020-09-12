// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ProceduralStructuredVolume.h"
#include "procedural_functions.h"

using namespace rkcommon;

namespace openvkl {
  namespace testing {

    template <
        typename VOXEL_TYPE = VoidType /* should be void (we have static_assert
                                to prevent such instantiation), but isn't due
                                to Windows Visual Studio compiler bug */
        ,
        VOXEL_TYPE samplingFunction(const vec3f &, float) = samplingNotImplemented,
        vec3f gradientFunction(const vec3f &)      = gradientNotImplemented>
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

      static void generateGridParameters(const vec3i &dimensions,
                                         const float boundingBoxSize,
                                         vec3f &gridOrigin,
                                         vec3f &gridSpacing);
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
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
              "structuredSpherical", dimensions, gridOrigin, gridSpacing)
    {
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
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

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline void ProceduralStructuredSphericalVolume<
        VOXEL_TYPE,
        samplingFunction,
        gradientFunction>::generateGridParameters(const vec3i &dimensions,
                                                  const float boundingBoxSize,
                                                  vec3f &gridOrigin,
                                                  vec3f &gridSpacing)
    {
      // generate grid parameters for a bounding box centered at (0,0,0) with a
      // maximum length boundingBoxSize

      constexpr float epsilon = std::numeric_limits<float>::epsilon();

      gridOrigin  = vec3f(0.f);
      gridSpacing = vec3f(0.5f * boundingBoxSize / (dimensions.x - 1),
                          180.f / (dimensions.y - 1) - epsilon,
                          360.f / (dimensions.z - 1) - epsilon);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume types ////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <typename VOXEL_TYPE>
    using WaveletStructuredSphericalVolume =
        ProceduralStructuredSphericalVolume<VOXEL_TYPE,
                                            getWaveletValue<VOXEL_TYPE>,
                                            getWaveletGradient>;

    template <typename VOXEL_TYPE>
    using XYZStructuredSphericalVolume =
        ProceduralStructuredSphericalVolume<VOXEL_TYPE,
                                            getXYZValue<VOXEL_TYPE>,
                                            getXYZGradient>;

    using RadiusProceduralVolume =
        ProceduralStructuredSphericalVolume<float, getRadiusValue>;

    // required due to Windows Visual Studio compiler bugs, which prevent us
    // from writing e.g. WaveletStructuredSphericalVolume<float>
    using WaveletStructuredSphericalVolumeUChar =
        ProceduralStructuredSphericalVolume<unsigned char,
                                            getWaveletValue<unsigned char>,
                                            getWaveletGradient>;
    using WaveletStructuredSphericalVolumeShort =
        ProceduralStructuredSphericalVolume<short,
                                            getWaveletValue<short>,
                                            getWaveletGradient>;
    using WaveletStructuredSphericalVolumeUShort =
        ProceduralStructuredSphericalVolume<unsigned short,
                                            getWaveletValue<unsigned short>,
                                            getWaveletGradient>;
    using WaveletStructuredSphericalVolumeFloat =
        ProceduralStructuredSphericalVolume<float,
                                            getWaveletValue<float>,
                                            getWaveletGradient>;
    using WaveletStructuredSphericalVolumeDouble =
        ProceduralStructuredSphericalVolume<double,
                                            getWaveletValue<double>,
                                            getWaveletGradient>;

  }  // namespace testing
}  // namespace openvkl
