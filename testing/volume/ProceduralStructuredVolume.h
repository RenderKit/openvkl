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
          const TemporalConfig &temporalConfig   = TemporalConfig(1),
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0)
          : TestingStructuredVolume(gridType,
                                    dimensions,
                                    gridOrigin,
                                    gridSpacing,
                                    voxelType,
                                    temporalConfig,
                                    dataCreationFlags,
                                    byteStride),
            ProceduralVolume(temporalConfig.numTimesteps > 1)
      {
      }

      virtual vec3f transformLocalToObjectCoordinates(
          const vec3f &localCoordinates) = 0;
    };

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &) = gradientNotImplemented>
    struct ProceduralStructuredVolume : public ProceduralStructuredVolumeBase
    {
      ProceduralStructuredVolume(
          const std::string &gridType,
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          const TemporalConfig &temporalConfig   = TemporalConfig(1),
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0);

      std::vector<unsigned char> generateVoxels() override;

     protected:
      float computeProceduralValueImpl(const vec3f &objectCoordinates,
                                       float time) const override;

      VOXEL_TYPE computeProceduralValueTyped(const vec3f &objectCoordinates,
                                             float time) const;

      vec3f computeProceduralGradientImpl(
          const vec3f &objectCoordinates) const override;
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
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline ProceduralStructuredVolume<VOXEL_TYPE,
                                      samplingFunction,
                                      gradientFunction>::

        ProceduralStructuredVolume(const std::string &gridType,
                                   const vec3i &dimensions,
                                   const vec3f &gridOrigin,
                                   const vec3f &gridSpacing,
                                   const TemporalConfig &temporalConfig,
                                   VKLDataCreationFlags dataCreationFlags,
                                   size_t byteStride)
        : ProceduralStructuredVolumeBase(gridType,
                                         dimensions,
                                         gridOrigin,
                                         gridSpacing,
                                         getVKLDataType<VOXEL_TYPE>(),
                                         temporalConfig,
                                         dataCreationFlags,
                                         byteStride)
    {
      // should be void, but isn't due to Windows Visual Studio compiler bug
      static_assert(!std::is_same<VOXEL_TYPE, VoidType>::value,
                    "must specify VOXEL_TYPE for ProceduralStructuredVolume");
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline std::vector<unsigned char>
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        generateVoxels()
    {
      {
        auto numValues = this->dimensions.long_product();

        std::vector<unsigned char> voxels(numValues * byteStride *
                                          temporalConfig.numTimesteps);

        rkcommon::tasking::parallel_for(this->dimensions.z, [&](int z) {
          for (size_t y = 0; y < this->dimensions.y; y++) {
            for (size_t x = 0; x < this->dimensions.x; x++) {
              for (size_t t = 0; t < temporalConfig.numTimesteps; t++) {
                size_t index =
                    temporalConfig.numTimesteps * size_t(z) *
                        this->dimensions.y * this->dimensions.x +
                    temporalConfig.numTimesteps * y * this->dimensions.x +
                    temporalConfig.numTimesteps * x + t;
                VOXEL_TYPE *voxelTyped =
                    (VOXEL_TYPE *)(voxels.data() + index * byteStride);
                vec3f objectCoordinates =
                    transformLocalToObjectCoordinates(vec3f(x, y, z));
                if (temporalConfig.timeSamples.empty()) {
                  *voxelTyped =
                      t == 0
                          ? samplingFunction(objectCoordinates, 0.f)
                          : samplingFunction(
                                objectCoordinates,
                                (float)t /
                                    (float)(temporalConfig.numTimesteps - 1));
                } else {
                  *voxelTyped = samplingFunction(objectCoordinates,
                                                 temporalConfig.timeSamples[t]);
                }
              }
            }
          }
        });

        return voxels;
      }
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline float
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValueImpl(const vec3f &objectCoordinates,
                                   float time) const
    {
      return float(computeProceduralValueTyped(objectCoordinates, time));
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline VOXEL_TYPE
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValueTyped(const vec3f &objectCoordinates,
                                    float time) const
    {
      return samplingFunction(objectCoordinates, time);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralGradientImpl(const vec3f &objectCoordinates) const
    {
      return gradientFunction(objectCoordinates);
    }

  }  // namespace testing
}  // namespace openvkl
