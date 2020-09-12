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

      virtual float computeProceduralValueMB(
          const vec3f &objectCoordinates,
          float time) const = 0;

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
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0);

      float computeProceduralValue(
          const vec3f &objectCoordinates) const override;

      VOXEL_TYPE computeProceduralValueTyped(
          const vec3f &objectCoordinates) const;

      vec3f computeProceduralGradient(
          const vec3f &objectCoordinates) const override;

      float computeProceduralValueMB(
          const vec3f &objectCoordinates,
          float time) const override;
        
      VOXEL_TYPE computeProceduralValueTypedMB(
          const vec3f &objectCoordinates,
          float time) const;

      std::vector<unsigned char> generateVoxels(
          size_t numTimeSamples = 1,
          const std::vector<float>& timeSamples = std::vector<float>()) override;
      std::vector<float> generateTimeData(
          size_t numTimeSamples = 1,
          const std::vector<float>& timeSamples = std::vector<float>()) override;
      std::vector<unsigned int> generateTimeConfig(
          size_t numTimeSamples = 1,
          const std::vector<float>& timeSamples = std::vector<float>()) override;
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
    inline float ProceduralStructuredVolume<
        VOXEL_TYPE, 
        samplingFunction, 
        gradientFunction>::computeProceduralValueMB(const vec3f &objectCoordinates,
                                                    float time) const
    {
      return float(computeProceduralValueTypedMB(objectCoordinates, time));
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline VOXEL_TYPE ProceduralStructuredVolume<
        VOXEL_TYPE, 
        samplingFunction, 
        gradientFunction>::computeProceduralValueTypedMB(const vec3f &objectCoordinates,
                                                         float time) const
    {
      return samplingFunction(objectCoordinates, time);
    }

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
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline float
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValue(const vec3f &objectCoordinates) const
    {
      return float(computeProceduralValueTyped(objectCoordinates));
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline VOXEL_TYPE
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValueTyped(const vec3f &objectCoordinates) const
    {
      return samplingFunction(objectCoordinates, 0.f);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralGradient(const vec3f &objectCoordinates) const
    {
      return gradientFunction(objectCoordinates);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline std::vector<unsigned char>
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        generateVoxels(size_t numTimeSamples,
                       const std::vector<float>& timeSamples)
    {
      {
        if (!timeSamples.empty() && numTimeSamples != timeSamples.size()) {
          throw std::runtime_error("timeSamples size not equal to numTimeSamples");
        }
        auto numValues = this->dimensions.long_product();
        std::vector<unsigned char> voxels(numValues * byteStride * numTimeSamples);

        rkcommon::tasking::parallel_for(this->dimensions.z, [&](int z) {
          for (size_t y = 0; y < this->dimensions.y; y++) {
            for (size_t x = 0; x < this->dimensions.x; x++) {
              for (size_t t = 0; t < numTimeSamples; t++) {
                size_t index =
                    numTimeSamples * size_t(z) * this->dimensions.y * this->dimensions.x +
                    numTimeSamples * y * this->dimensions.x + numTimeSamples * x + t;
                VOXEL_TYPE *voxelTyped =
                    (VOXEL_TYPE *)(voxels.data() + index * byteStride);
                vec3f objectCoordinates =
                    transformLocalToObjectCoordinates(vec3f(x, y, z));
                if (timeSamples.empty()) {
                  *voxelTyped = t==0 ? 
                      samplingFunction(objectCoordinates, 0.f) : 
                      samplingFunction(objectCoordinates, (float)t/(float)(numTimeSamples-1));
                } else {
                  *voxelTyped = samplingFunction(objectCoordinates, timeSamples[t]);
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
    inline std::vector<float>
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        generateTimeData(size_t numTimeSamples,
                         const std::vector<float>& timeSamples)
    {
      if (!timeSamples.empty() && numTimeSamples != timeSamples.size()) {
        throw std::runtime_error("timeSamples size not equal to numTimeSamples");
      }
      if (numTimeSamples == 1 || timeSamples.empty()) {
        return std::vector<float>();
      }

      auto numValues = this->dimensions.long_product();
      std::vector<float> timeData(numValues * numTimeSamples);
      rkcommon::tasking::parallel_for(this->dimensions.z, [&](int z) {
        for (size_t y = 0; y < this->dimensions.y; y++) {
          for (size_t x = 0; x < this->dimensions.x; x++) {
            for (size_t t = 0; t < timeSamples.size(); t++) {
              size_t index =
                  numTimeSamples * size_t(z) * this->dimensions.y * this->dimensions.x +
                  numTimeSamples * y * this->dimensions.x + numTimeSamples * x + t;
              timeData[index] = timeSamples[t];
            }
          }
        }
      });

      return timeData;
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &)>
    inline std::vector<unsigned int>
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        generateTimeConfig(size_t numTimeSamples,
                           const std::vector<float>& timeSamples)
    {
      if (!timeSamples.empty() && numTimeSamples != timeSamples.size()) {
        throw std::runtime_error("timeSamples size not equal to numTimeSamples");
      }
      if (timeSamples.empty()) {
        return std::vector<unsigned int>(1,numTimeSamples);
      }

      auto numValues = this->dimensions.long_product();
      std::vector<unsigned int> timeConfig(numValues+1);
      unsigned int indexSum = 0;
      for (size_t z = 0; z < this->dimensions.z; ++z) {
        for (size_t y = 0; y < this->dimensions.y; y++) {
          for (size_t x = 0; x < this->dimensions.x; x++) {
            size_t index =
                size_t(z) * this->dimensions.y * this->dimensions.x +
                y * this->dimensions.x + x;
            timeConfig[index] = indexSum;
            indexSum += numTimeSamples;
          }
        }
      }
      timeConfig[numValues] = indexSum;

      return timeConfig;
    }

  }  // namespace testing
}  // namespace openvkl
