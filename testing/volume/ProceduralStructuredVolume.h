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
      ProceduralStructuredVolumeBase(const std::string &gridType,
                                     const vec3i &dimensions,
                                     const vec3f &gridOrigin,
                                     const vec3f &gridSpacing,
                                     const TemporalConfig &temporalConfig,
                                     VKLDataType voxelType,
                                     VKLDataCreationFlags dataCreationFlags,
                                     size_t byteStride)
          : TestingStructuredVolume(gridType,
                                    dimensions,
                                    gridOrigin,
                                    gridSpacing,
                                    temporalConfig,
                                    voxelType,
                                    dataCreationFlags,
                                    byteStride),
            ProceduralVolume(temporalConfig.hasTime())
      {
      }

      virtual vec3f transformLocalToObjectCoordinates(
          const vec3f &localCoordinates) const = 0;
    };

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float) =
                  gradientNotImplemented>
    struct ProceduralStructuredVolume : public ProceduralStructuredVolumeBase
    {
      ProceduralStructuredVolume(
          const std::string &gridType,
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          const TemporalConfig &temporalConfig   = TemporalConfig(),
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0);

      void generateVoxels(std::vector<unsigned char> &voxels,
                          std::vector<float> &time,
                          std::vector<uint32_t> &tuvIndex) const override final;

     protected:
      float computeProceduralValueImpl(const vec3f &objectCoordinates,
                                       float time) const override;

      VOXEL_TYPE computeProceduralValueTyped(const vec3f &objectCoordinates,
                                             float time) const;

      vec3f computeProceduralGradientImpl(const vec3f &objectCoordinates,
                                          float time) const override;
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
              vec3f gradientFunction(const vec3f &, float)>
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
                                         temporalConfig,
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
              vec3f gradientFunction(const vec3f &, float)>
    inline void
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        generateVoxels(std::vector<unsigned char> &voxels,
                       std::vector<float> &time,
                       std::vector<uint32_t> &tuvIndex) const
    {
      const size_t numVoxels    = dimensions.long_product();
      const size_t numBytes     = numVoxels * byteStride;
      const size_t numTimesteps = temporalConfig.sampleTime.size();

      if (temporalConfig.type == TemporalConfig::Constant) {
        voxels.resize(numBytes);
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
              *voxelTyped = samplingFunction(objectCoordinates, 0.f);
            }
          }
        });
      }

      else if (temporalConfig.type == TemporalConfig::Structured) {
        voxels.resize(numBytes * numTimesteps);
        rkcommon::tasking::parallel_for(this->dimensions.z, [&](int z) {
          for (size_t y = 0; y < this->dimensions.y; y++) {
            for (size_t x = 0; x < this->dimensions.x; x++) {
              for (size_t t = 0; t < numTimesteps; t++) {
                size_t index = numTimesteps * size_t(z) * this->dimensions.y *
                                   this->dimensions.x +
                               numTimesteps * y * this->dimensions.x +
                               numTimesteps * x + t;
                VOXEL_TYPE *voxelTyped =
                    (VOXEL_TYPE *)(voxels.data() + index * byteStride);
                vec3f objectCoordinates =
                    transformLocalToObjectCoordinates(vec3f(x, y, z));
                *voxelTyped = samplingFunction(objectCoordinates,
                                               temporalConfig.sampleTime[t]);
              }
            }
          }
        });
      }

      else if (temporalConfig.type == TemporalConfig::Unstructured &&
               temporalConfig.numRefitSamples > 1) {
        const size_t numRefitSamples = temporalConfig.numRefitSamples;
        voxels.resize(numBytes * numRefitSamples);
        time.resize(numVoxels * numRefitSamples);
        tuvIndex.resize(numVoxels + 1);
        rkcommon::tasking::parallel_for(this->dimensions.z, [&](int z) {
          std::vector<VOXEL_TYPE> refitBuffer(numTimesteps);
          for (size_t y = 0; y < this->dimensions.y; y++) {
            for (size_t x = 0; x < this->dimensions.x; x++) {
              // First, sample our field into a temporary buffer.
              for (size_t t = 0; t < numTimesteps; ++t) {
                vec3f objectCoordinates =
                    transformLocalToObjectCoordinates(vec3f(x, y, z));
                refitBuffer[t] = samplingFunction(objectCoordinates,
                                                  temporalConfig.sampleTime[t]);
              }

              // Attempt to shrink the time range.
              size_t last = numTimesteps - 1;
              if (refitBuffer[last] == 0) {
                while (last > 2 && refitBuffer[last - 1] == 0) {
                  --last;
                }
              }

              size_t first = 0;
              if (refitBuffer[first] == 0) {
                while ((first + 2) < last &&
                       refitBuffer[first + 1] == 0) {
                  ++first;
                }
              }

              // Now resample with this new time range.
              const float t0 = temporalConfig.sampleTime[first];
              const float t1 = temporalConfig.sampleTime[last];
              const float dt = (t1 - t0) / static_cast<float>(numRefitSamples-1);

              const size_t voxelIndex =
                  size_t(z) * this->dimensions.y * this->dimensions.x +
                  y * this->dimensions.x + x;
              tuvIndex[voxelIndex] = numRefitSamples * voxelIndex;

              for (size_t t = 0; t < numRefitSamples; t++) {
                const size_t index = numRefitSamples * voxelIndex + t;
                VOXEL_TYPE *voxelTyped =
                    (VOXEL_TYPE *)(voxels.data() + index * byteStride);
                vec3f objectCoordinates =
                    transformLocalToObjectCoordinates(vec3f(x, y, z));
                const float sampleTime = t0 + t * dt;
                *voxelTyped = samplingFunction(objectCoordinates, sampleTime);
                time[index] = sampleTime;
              }
            }
          }
        });
        *tuvIndex.rbegin() = time.size();
      }

      else if (temporalConfig.type == TemporalConfig::Unstructured) {
        voxels.resize(numBytes * numTimesteps);
        time.resize(numVoxels * numTimesteps);
        tuvIndex.resize(numVoxels + 1);
        rkcommon::tasking::parallel_for(this->dimensions.z, [&](int z) {
          for (size_t y = 0; y < this->dimensions.y; y++) {
            for (size_t x = 0; x < this->dimensions.x; x++) {
              const size_t voxelIndex =
                  size_t(z) * this->dimensions.y * this->dimensions.x +
                  y * this->dimensions.x + x;
              tuvIndex[voxelIndex] = numTimesteps * voxelIndex;
              for (size_t t = 0; t < numTimesteps; t++) {
                const size_t index = numTimesteps * voxelIndex + t;
                VOXEL_TYPE *voxelTyped =
                    (VOXEL_TYPE *)(voxels.data() + index * byteStride);
                vec3f objectCoordinates =
                    transformLocalToObjectCoordinates(vec3f(x, y, z));
                *voxelTyped = samplingFunction(objectCoordinates,
                                               temporalConfig.sampleTime[t]);
                time[index] = temporalConfig.sampleTime[t];
              }
            }
          }
        });
        *tuvIndex.rbegin() = time.size();
      }
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline float
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValueImpl(const vec3f &objectCoordinates,
                                   float time) const
    {
      return float(computeProceduralValueTyped(objectCoordinates, time));
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline VOXEL_TYPE
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValueTyped(const vec3f &objectCoordinates,
                                    float time) const
    {
      return samplingFunction(objectCoordinates, time);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline vec3f
    ProceduralStructuredVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralGradientImpl(const vec3f &objectCoordinates,
                                      float time) const
    {
      return gradientFunction(objectCoordinates, time);
    }

  }  // namespace testing
}  // namespace openvkl
