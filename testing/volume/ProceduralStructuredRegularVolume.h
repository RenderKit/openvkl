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

#include "ProceduralStructuredVolume.h"
#include "procedural_functions.h"

using namespace ospcommon;

namespace openvkl {
  namespace testing {

    template <typename VOXEL_TYPE =
                  VoidType /* should be void (we have static_assert to
                          prevent such instantiation), but isn't due
                          to Windows Visual Studio compiler bug */
              ,
              VOXEL_TYPE samplingFunction(const vec3f &) =
                  samplingNotImplemented,
              vec3f gradientFunction(const vec3f &) = gradientNotImplemented>
    struct ProceduralStructuredRegularVolume
        : public ProceduralStructuredVolume<VOXEL_TYPE,
                                            samplingFunction,
                                            gradientFunction>
    {
      ProceduralStructuredRegularVolume(const vec3i &dimensions,
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
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                             samplingFunction,
                                             gradientFunction>::
        ProceduralStructuredRegularVolume(const vec3i &dimensions,
                                          const vec3f &gridOrigin,
                                          const vec3f &gridSpacing)
        : ProceduralStructuredVolume<VOXEL_TYPE,
                                     samplingFunction,
                                     gradientFunction>(
              "structuredRegular", dimensions, gridOrigin, gridSpacing)
    {
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                                   samplingFunction,
                                                   gradientFunction>::
        transformLocalToObjectCoordinates(const vec3f &localCoordinates)
    {
      return this->gridOrigin + localCoordinates * this->gridSpacing;
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline void ProceduralStructuredRegularVolume<
        VOXEL_TYPE,
        samplingFunction,
        gradientFunction>::generateGridParameters(const vec3i &dimensions,
                                                  const float boundingBoxSize,
                                                  vec3f &gridOrigin,
                                                  vec3f &gridSpacing)
    {
      // generate grid parameters for a bounding box centered at (0,0,0) with a
      // maximum length boundingBoxSize

      const float minGridSpacing =
          reduce_min(boundingBoxSize / (dimensions - 1));

      gridOrigin  = -0.5f * (dimensions - 1) * minGridSpacing;
      gridSpacing = vec3f(minGridSpacing);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume types ////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <typename VOXEL_TYPE>
    using WaveletStructuredRegularVolume =
        ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                          getWaveletValue<VOXEL_TYPE>,
                                          getWaveletGradient>;

    template <typename VOXEL_TYPE>
    using XYZStructuredRegularVolume =
        ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                          getXYZValue<VOXEL_TYPE>,
                                          getXYZGradient>;

    using ZProceduralVolume =
        ProceduralStructuredRegularVolume<float, getZValue, getZGradient>;

    // required due to Windows Visual Studio compiler bugs, which prevent us
    // from writing e.g. WaveletStructuredRegularVolume<float>
    using WaveletStructuredRegularVolumeUChar =
        ProceduralStructuredRegularVolume<unsigned char,
                                          getWaveletValue<unsigned char>,
                                          getWaveletGradient>;
    using WaveletStructuredRegularVolumeShort =
        ProceduralStructuredRegularVolume<short,
                                          getWaveletValue<short>,
                                          getWaveletGradient>;
    using WaveletStructuredRegularVolumeUShort =
        ProceduralStructuredRegularVolume<unsigned short,
                                          getWaveletValue<unsigned short>,
                                          getWaveletGradient>;
    using WaveletStructuredRegularVolumeFloat =
        ProceduralStructuredRegularVolume<float,
                                          getWaveletValue<float>,
                                          getWaveletGradient>;
    using WaveletStructuredRegularVolumeDouble =
        ProceduralStructuredRegularVolume<double,
                                          getWaveletValue<double>,
                                          getWaveletGradient>;

  }  // namespace testing
}  // namespace openvkl
