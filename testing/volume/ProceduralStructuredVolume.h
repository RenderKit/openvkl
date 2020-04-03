// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TestingStructuredVolume.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
// std
#include <algorithm>

using namespace ospcommon;

namespace openvkl {
  namespace testing {

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &) = gradientNotImplemented>
    struct ProceduralStructuredVolume : public TestingStructuredVolume
    {
      ProceduralStructuredVolume(const std::string &gridType,
                                 const vec3i &dimensions,
                                 const vec3f &gridOrigin,
                                 const vec3f &gridSpacing);

      VOXEL_TYPE computeProceduralValue(const vec3f &objectCoordinates);

      vec3f computeProceduralGradient(const vec3f &objectCoordinates);

      std::vector<unsigned char> generateVoxels() override;

      virtual vec3f transformLocalToObjectCoordinates(
          const vec3f &localCoordinates) = 0;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    struct VoidType
    {
    };

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline ProceduralStructuredVolume<VOXEL_TYPE,
                                      samplingFunction,
                                      gradientFunction>::
        ProceduralStructuredVolume(const std::string &gridType,
                                   const vec3i &dimensions,
                                   const vec3f &gridOrigin,
                                   const vec3f &gridSpacing)
        : TestingStructuredVolume(gridType,
                                  dimensions,
                                  gridOrigin,
                                  gridSpacing,
                                  getVKLDataType<VOXEL_TYPE>())
    {
      // should be void, but isn't due to Windows Visual Studio compiler bug
      static_assert(!std::is_same<VOXEL_TYPE, VoidType>::value,
                    "must specify VOXEL_TYPE for ProceduralStructuredVolume");
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline VOXEL_TYPE
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValue(const vec3f &objectCoordinates)
    {
      return samplingFunction(objectCoordinates);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralGradient(const vec3f &objectCoordinates)
    {
      return gradientFunction(objectCoordinates);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline std::vector<unsigned char>
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        generateVoxels()
    {
      {
        auto numValues = this->dimensions.long_product();
        std::vector<unsigned char> voxels(numValues * sizeof(VOXEL_TYPE));

        VOXEL_TYPE *voxelsTyped = (VOXEL_TYPE *)voxels.data();

        ospcommon::tasking::parallel_for(this->dimensions.z, [&](int z) {
          for (size_t y = 0; y < this->dimensions.y; y++) {
            for (size_t x = 0; x < this->dimensions.x; x++) {
              size_t index = z * this->dimensions.y * this->dimensions.x +
                             y * this->dimensions.x + x;
              vec3f objectCoordinates =
                  transformLocalToObjectCoordinates(vec3f(x, y, z));
              voxelsTyped[index] = samplingFunction(objectCoordinates);
            }
          }
        });

        return voxels;
      }
    }

  }  // namespace testing
}  // namespace openvkl
