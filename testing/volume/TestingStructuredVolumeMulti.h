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
          const TemporalConfig &temporalConfig,
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
                                      unsigned int attributeIndex,
                                      float time = 0.f) const;

      std::string getGridType() const;
      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;
      TemporalConfig getTemporalConfig() const;

      unsigned int getNumAttributes();

      vec3f transformLocalToObjectCoordinates(const vec3f &localCoordinates);

     protected:
      void generateVKLVolume() override final;

      std::string gridType;
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      TemporalConfig temporalConfig;
      std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>>
          attributeVolumes;
      VKLDataCreationFlags dataCreationFlags;
      bool useAOSLayout;

      // data may need to be retained for shared data buffers
      std::vector<std::vector<unsigned char>> voxels;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline TestingStructuredVolumeMulti::TestingStructuredVolumeMulti(
        const std::string &gridType,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        const TemporalConfig &temporalConfig,
        const std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>>
            &attributeVolumes,
        VKLDataCreationFlags dataCreationFlags,
        bool useAOSLayout)
        : gridType(gridType),
          dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing),
          temporalConfig(temporalConfig),
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
                          (v->getGridSpacing() == gridSpacing) &&
                          (v->getTemporalConfig() == temporalConfig);

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
        const vec3f &objectCoordinates, 
        unsigned int attributeIndex,
        float time) const
    {
      return attributeVolumes[attributeIndex]->computeProceduralGradient(
          objectCoordinates, time);
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

    inline TemporalConfig TestingStructuredVolumeMulti::getTemporalConfig()
        const
    {
      return temporalConfig;
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

      std::vector<VKLData> attributesData;

      if (useAOSLayout) {
        if (temporalConfig.hasTime()) {
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

          assert(av.size() == dimensions.long_product() * voxelSizes[i]);

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

          assert(voxels[i].size() ==
                 dimensions.long_product() * temporalConfig.numTimesteps *
                     sizeOfVKLDataType(attributeVolumes[i]->getVoxelType()));

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

      if (temporalConfig.hasStructuredTime()) {
        vklSetInt(volume,
                  "temporallyStructuredNumTimesteps",
                  temporalConfig.getStructuredNumTimesteps());
      } else if (temporalConfig.hasUnstructuredTime()) {
        VKLData indicesData = temporalConfig.generateUnstructuredIndicesData(
            dimensions.long_product());
        vklSetData(volume, "temporallyUnstructuredIndices", indicesData);
        vklRelease(indicesData);

        VKLData timesData = temporalConfig.generateUnstructuredTimesData(
            dimensions.long_product());
        vklSetData(volume, "temporallyUnstructuredTimes", timesData);
        vklRelease(timesData);
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
        const TemporalConfig &temporalConfig,
        VKLDataCreationFlags dataCreationFlags,
        bool useAOSLayout)
    {
      std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>> volumes;

      volumes.push_back(std::make_shared<WaveletStructuredRegularVolumeFloat>(
          dimensions, gridOrigin, gridSpacing, temporalConfig));

      volumes.push_back(std::make_shared<XProceduralVolume>(
          dimensions, gridOrigin, gridSpacing, temporalConfig));

      volumes.push_back(std::make_shared<YProceduralVolume>(
          dimensions, gridOrigin, gridSpacing, temporalConfig));

      volumes.push_back(std::make_shared<ZProceduralVolume>(
          dimensions, gridOrigin, gridSpacing, temporalConfig));

      return new TestingStructuredVolumeMulti("structuredRegular",
                                              dimensions,
                                              gridOrigin,
                                              gridSpacing,
                                              temporalConfig,
                                              volumes,
                                              dataCreationFlags,
                                              useAOSLayout);
    }

    // only has volume attributes suitable for functional testing against
    // procedural gradients
    inline TestingStructuredVolumeMulti *
    generateMultiAttributeStructuredRegularVolumeMBGradients(
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        const TemporalConfig &temporalConfig,
        VKLDataCreationFlags dataCreationFlags,
        bool useAOSLayout)
    {
      std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>> volumes;

      volumes.push_back(std::make_shared<XProceduralVolume>(
          dimensions, gridOrigin, gridSpacing, temporalConfig));

      volumes.push_back(std::make_shared<YProceduralVolume>(
          dimensions, gridOrigin, gridSpacing, temporalConfig));

      volumes.push_back(std::make_shared<ZProceduralVolume>(
          dimensions, gridOrigin, gridSpacing, temporalConfig));

      return new TestingStructuredVolumeMulti("structuredRegular",
                                              dimensions,
                                              gridOrigin,
                                              gridSpacing,
                                              temporalConfig,
                                              volumes,
                                              dataCreationFlags,
                                              useAOSLayout);
    }

  }  // namespace testing
}  // namespace openvkl
