// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <numeric>
#include <vector>
// openvkl
#include "TestingVolume.h"

namespace openvkl {
  namespace testing {

    inline std::vector<unsigned int> getAttributeIndices(
        unsigned int numAttributes)
    {
      std::vector<unsigned int> attributeIndices(numAttributes);
      std::iota(attributeIndices.begin(), attributeIndices.end(), 0);

      return attributeIndices;
    }

    struct TestingStructuredVolumeMulti : public TestingVolume
    {
      TestingStructuredVolumeMulti(
          const std::string &gridType,
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          const std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>>
              &attributeVolumes,
          VKLDataCreationFlags dataCreationFlags,
          bool useAOSLayout);

      // maps to first attribute only
      range1f getComputedValueRange() const override;

      range1f getComputedValueRange(unsigned int attributeIndex) const;

      float computeProceduralValue(const vec3f &objectCoordinates,
                                   unsigned int attributeIndex,
                                   float time = 0.f) const;

      vec3f computeProceduralGradient(const vec3f &objectCoordinates,
                                      unsigned int attributeIndex) const;

      std::string getGridType() const;
      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;

      unsigned int getNumAttributes();

      vec3f transformLocalToObjectCoordinates(const vec3f &localCoordinates);

     protected:
      void generateVKLVolume() override final;

      std::string gridType;
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>>
          attributeVolumes;
      VKLDataCreationFlags dataCreationFlags;
      bool useAOSLayout;

      // data may need to be retained for shared data buffers
      std::vector<std::vector<unsigned char>> voxels;
      std::vector<std::vector<uint8_t>> timeConfig;
      std::vector<std::vector<float>> timeData;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline TestingStructuredVolumeMulti::TestingStructuredVolumeMulti(
        const std::string &gridType,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        const std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>>
            &attributeVolumes,
        VKLDataCreationFlags dataCreationFlags,
        bool useAOSLayout)
        : gridType(gridType),
          dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing),
          attributeVolumes(attributeVolumes),
          dataCreationFlags(dataCreationFlags),
          useAOSLayout(useAOSLayout)
    {
      if (attributeVolumes.size() == 0) {
        throw std::runtime_error("no provided attribute volumes");
      }

      // verify provided attribute volumes are consistent with the provided
      // parameters
      for (const auto &v : attributeVolumes) {
        bool compatible = (v->getGridType() == gridType) &&
                          (v->getDimensions() == dimensions) &&
                          (v->getGridOrigin() == gridOrigin) &&
                          (v->getGridSpacing() == gridSpacing);

        if (!compatible) {
          throw std::runtime_error(
              "a provided attribute volume is not compatible with the "
              "constructed TestingStructuredVolumeMulti instance");
        }
      }
    }

    inline range1f TestingStructuredVolumeMulti::getComputedValueRange() const
    {
      return attributeVolumes[0]->getComputedValueRange();
    }

    inline range1f TestingStructuredVolumeMulti::getComputedValueRange(
        unsigned int attributeIndex) const
    {
      return attributeVolumes[attributeIndex]->getComputedValueRange();
    }

    inline float TestingStructuredVolumeMulti::computeProceduralValue(
        const vec3f &objectCoordinates,
        unsigned int attributeIndex,
        float time) const
    {
      return attributeVolumes[attributeIndex]->computeProceduralValue(
          objectCoordinates, time);
    }

    inline vec3f TestingStructuredVolumeMulti::computeProceduralGradient(
        const vec3f &objectCoordinates, unsigned int attributeIndex) const
    {
      return attributeVolumes[attributeIndex]->computeProceduralGradient(
          objectCoordinates);
    }

    inline std::string TestingStructuredVolumeMulti::getGridType() const
    {
      return gridType;
    }

    inline vec3i TestingStructuredVolumeMulti::getDimensions() const
    {
      return dimensions;
    }

    inline vec3f TestingStructuredVolumeMulti::getGridOrigin() const
    {
      return gridOrigin;
    }

    inline vec3f TestingStructuredVolumeMulti::getGridSpacing() const
    {
      return gridSpacing;
    }

    inline unsigned int TestingStructuredVolumeMulti::getNumAttributes()
    {
      return attributeVolumes.size();
    }

    inline vec3f
    TestingStructuredVolumeMulti::transformLocalToObjectCoordinates(
        const vec3f &localCoordinates)
    {
      // at construction we're guaranteed all attribute volumes have the same
      // grid parameters, so we can simply do the transformation on the first
      // volume
      return attributeVolumes[0]->transformLocalToObjectCoordinates(
          localCoordinates);
    }

    inline void TestingStructuredVolumeMulti::generateVKLVolume()
    {
      volume = vklNewVolume(gridType.c_str());

      vklSetVec3i(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetVec3f(
          volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vklSetVec3f(
          volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

      voxels.resize(attributeVolumes.size());

      // determine if any of the attribute volumes have multiple time steps
      bool haveTimesteps = false;

      for (const auto &av : attributeVolumes) {
        if (av->getTemporalConfig().numTimesteps > 1) {
          haveTimesteps = true;
          break;
        }
      }

      std::vector<VKLData> attributesData;

      if (useAOSLayout) {
        if (haveTimesteps) {
          throw std::runtime_error(
              "TestingStructuredVolumeMulti does not support AOS layout with "
              "time-varying attribute volumes");
        }

        // voxel size per attribute
        std::vector<size_t> voxelSizes;

        for (const auto &av : attributeVolumes) {
          voxelSizes.push_back(sizeOfVKLDataType(av->getVoxelType()));
        }

        // voxel size for all attributes combined
        const size_t voxelSizeSum =
            std::accumulate(voxelSizes.begin(), voxelSizes.end(), 0);

        // combined voxel data for all attributes, AOS layout
        auto &v = voxels[0];
        v.resize(voxelSizeSum * dimensions.long_product());

        // for each attribute: populate data into combined AOS voxel buffer, and
        // create (strided) VKL data object
        for (size_t i = 0; i < attributeVolumes.size(); i++) {
          const size_t voxelSizeOffset =
              std::accumulate(voxelSizes.begin(), voxelSizes.begin() + i, 0);

          std::vector<unsigned char> av = attributeVolumes[i]->generateVoxels();

          for (size_t j = 0; j < dimensions.long_product(); j++) {
            std::memcpy(&v[j * voxelSizeSum + voxelSizeOffset],
                        &av[j * voxelSizes[i]],
                        voxelSizes[i]);
          }

          VKLData attributeData =
              vklNewData(dimensions.long_product(),
                         attributeVolumes[i]->getVoxelType(),
                         v.data() + voxelSizeOffset,
                         dataCreationFlags,
                         voxelSizeSum);

          attributesData.push_back(attributeData);
        }

        if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
          std::vector<unsigned char>().swap(v);
        }

      } else {
        // SOA layout, where each attribute's data is a compact array
        for (int i = 0; i < attributeVolumes.size(); i++) {
          voxels[i] = attributeVolumes[i]->generateVoxels();

          VKLData attributeData = vklNewData(
              dimensions.long_product() *
                  attributeVolumes[i]->getTemporalConfig().numTimesteps,
              attributeVolumes[i]->getVoxelType(),
              voxels[i].data(),
              dataCreationFlags,
              0);

          attributesData.push_back(attributeData);

          if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
            std::vector<unsigned char>().swap(voxels[i]);
          }
        }
      }

      VKLData data =
          vklNewData(attributesData.size(), VKL_DATA, attributesData.data());
      for (const auto &d : attributesData) {
        vklRelease(d);
      }
      vklSetData(volume, "data", data);
      vklRelease(data);

      // add time parameters if necessary
      if (haveTimesteps) {
        timeConfig.resize(attributeVolumes.size());
        timeData.resize(attributeVolumes.size());

        std::vector<VKLData> attributesTimeConfig;
        std::vector<VKLData> attributesTimeData;

        for (int i = 0; i < attributeVolumes.size(); i++) {
          timeConfig[i] = attributeVolumes[i]->generateTimeConfig();
          timeData[i]   = attributeVolumes[i]->generateTimeData();

          VKLData attributeTimeConfig = vklNewData(timeConfig[i].size(),
                                                   VKL_UCHAR,
                                                   timeConfig[i].data(),
                                                   dataCreationFlags,
                                                   0);
          attributesTimeConfig.push_back(attributeTimeConfig);

          if (timeData[i].size() == 0) {
            attributesTimeData.push_back(nullptr);
          } else {
            VKLData attributeTimeData = vklNewData(timeData[i].size(),
                                                   VKL_FLOAT,
                                                   timeData[i].data(),
                                                   dataCreationFlags,
                                                   0);
            attributesTimeData.push_back(attributeTimeData);
          }

          if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
            std::vector<uint8_t>().swap(timeConfig[i]);
            std::vector<float>().swap(timeData[i]);
          }
        }

        VKLData timeConfig = vklNewData(
            attributesTimeConfig.size(), VKL_DATA, attributesTimeConfig.data());
        VKLData timeData = vklNewData(
            attributesTimeData.size(), VKL_DATA, attributesTimeData.data());

        for (const auto &d : attributesTimeConfig) {
          vklRelease(d);
        }
        for (const auto &d : attributesTimeData) {
          if (d) {
            vklRelease(d);
          }
        }

        vklSetData(volume, "timeConfig", timeConfig);
        vklSetData(volume, "timeData", timeData);

        vklRelease(timeConfig);
        vklRelease(timeData);
      }

      vklCommit(volume);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume generation helpers ///////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    inline TestingStructuredVolumeMulti *
    generateMultiAttributeStructuredRegularVolume(
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        VKLDataCreationFlags dataCreationFlags,
        bool useAOSLayout)
    {
      std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>> volumes;

      volumes.push_back(std::make_shared<WaveletStructuredRegularVolumeFloat>(
          dimensions, gridOrigin, gridSpacing));

      volumes.push_back(std::make_shared<XProceduralVolume>(
          dimensions, gridOrigin, gridSpacing));

      volumes.push_back(std::make_shared<YProceduralVolume>(
          dimensions, gridOrigin, gridSpacing));

      volumes.push_back(std::make_shared<ZProceduralVolume>(
          dimensions, gridOrigin, gridSpacing));

      return new TestingStructuredVolumeMulti("structuredRegular",
                                              dimensions,
                                              gridOrigin,
                                              gridSpacing,
                                              volumes,
                                              dataCreationFlags,
                                              useAOSLayout);
    }

    inline TestingStructuredVolumeMulti *
    generateMultiAttributeStructuredRegularVolumeMB(
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        VKLDataCreationFlags dataCreationFlags)
    {
      std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>> volumes;

      volumes.push_back(std::make_shared<WaveletStructuredRegularVolumeFloat>(
          dimensions,
          gridOrigin,
          gridSpacing,
          TemporalConfig(1, std::vector<float>())));

      std::vector<float> timeSamples{0.f, 0.15f, 0.3f, 0.65f, 0.9f, 1.0f};
      volumes.push_back(std::make_shared<XYZStructuredRegularVolume<float>>(
          dimensions,
          gridOrigin,
          gridSpacing,
          TemporalConfig(timeSamples.size(), timeSamples)));

      volumes.push_back(std::make_shared<XYZStructuredRegularVolume<float>>(
          dimensions,
          gridOrigin,
          gridSpacing,
          TemporalConfig(3, std::vector<float>())));

      return new TestingStructuredVolumeMulti("structuredRegular",
                                              dimensions,
                                              gridOrigin,
                                              gridSpacing,
                                              volumes,
                                              dataCreationFlags,
                                              false);
    }

  }  // namespace testing
}  // namespace openvkl
