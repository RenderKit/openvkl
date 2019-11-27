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
              "structured_regular", dimensions, gridOrigin, gridSpacing)
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

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume types ////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    using WaveletProceduralVolumeUchar =
        ProceduralStructuredRegularVolume<unsigned char,
                                          getWaveletValue<unsigned char>>;

    using WaveletProceduralVolumeShort =
        ProceduralStructuredRegularVolume<short, getWaveletValue<short>>;

    using WaveletProceduralVolumeUshort =
        ProceduralStructuredRegularVolume<unsigned short,
                                          getWaveletValue<unsigned short>>;

    using WaveletProceduralVolumeFloat =
        ProceduralStructuredRegularVolume<float,
                                          getWaveletValue<float>,
                                          getWaveletGradient>;

    using WaveletProceduralVolumeDouble =
        ProceduralStructuredRegularVolume<double, getWaveletValue<double>>;

    using WaveletProceduralVolume = WaveletProceduralVolumeFloat;

    using ZProceduralVolume =
        ProceduralStructuredRegularVolume<float, getZValue, getZGradient>;

    using XYZProceduralVolume =
        ProceduralStructuredRegularVolume<float, getXYZValue, getXYZGradient>;

  }  // namespace testing
}  // namespace openvkl
