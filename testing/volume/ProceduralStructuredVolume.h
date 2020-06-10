// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ProceduralVolume.h"
#include "TestingStructuredVolume.h"
// rkcommon
#include "rkcommon/tasking/parallel_for.h"
// std
#include <algorithm>

using namespace rkcommon;

namespace openvkl {
  namespace testing {

    // allows handling of generic procedural structured volumes without
    // knowledge of template parameters
    struct ProceduralStructuredVolumeBase : public TestingStructuredVolume,
                                            public ProceduralVolume
    {
      ProceduralStructuredVolumeBase(
          const std::string &gridType,
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          VKLDataType voxelType,
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0)
          : TestingStructuredVolume(gridType,
                                    dimensions,
                                    gridOrigin,
                                    gridSpacing,
                                    voxelType,
                                    dataCreationFlags,
                                    byteStride)
      {
      }

      virtual vec3f transformLocalToObjectCoordinates(
          const vec3f &localCoordinates) = 0;
    };

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &) = gradientNotImplemented>
    struct ProceduralStructuredVolume : public ProceduralStructuredVolumeBase
    {
      ProceduralStructuredVolume(
          const std::string &gridType,
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0);

      float computeProceduralValue(
          const vec3f &objectCoordinates) const override;

      VOXEL_TYPE computeProceduralValueTyped(
          const vec3f &objectCoordinates) const;

      vec3f computeProceduralGradient(
          const vec3f &objectCoordinates) const override;

      std::vector<unsigned char> generateVoxels() override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    struct VoidType
    {
      // required for Windows Visual Studio compiler issues
      operator float() const
      {
        return 0.f;
      }
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
                                   const vec3f &gridSpacing,
                                   VKLDataCreationFlags dataCreationFlags,
                                   size_t byteStride)
        : ProceduralStructuredVolumeBase(gridType,
                                         dimensions,
                                         gridOrigin,
                                         gridSpacing,
                                         getVKLDataType<VOXEL_TYPE>(),
                                         dataCreationFlags,
                                         byteStride)
    {
      // should be void, but isn't due to Windows Visual Studio compiler bug
      static_assert(!std::is_same<VOXEL_TYPE, VoidType>::value,
                    "must specify VOXEL_TYPE for ProceduralStructuredVolume");
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline float
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValue(const vec3f &objectCoordinates) const
    {
      return float(computeProceduralValueTyped(objectCoordinates));
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline VOXEL_TYPE
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValueTyped(const vec3f &objectCoordinates) const
    {
      return samplingFunction(objectCoordinates);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralGradient(const vec3f &objectCoordinates) const
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
        std::vector<unsigned char> voxels(numValues * byteStride);

        rkcommon::tasking::parallel_for(this->dimensions.z, [&](int z) {
          for (size_t y = 0; y < this->dimensions.y; y++) {
            for (size_t x = 0; x < this->dimensions.x; x++) {
              size_t index =
                  size_t(z) * this->dimensions.y * this->dimensions.x +
                  y * this->dimensions.x + x;
              VOXEL_TYPE *voxelTyped =
                  (VOXEL_TYPE *)(voxels.data() + index * byteStride);
              vec3f objectCoordinates =
                  transformLocalToObjectCoordinates(vec3f(x, y, z));
              *voxelTyped = samplingFunction(objectCoordinates);
            }
          }
        });

        return voxels;
      }
    }

  }  // namespace testing
}  // namespace openvkl
