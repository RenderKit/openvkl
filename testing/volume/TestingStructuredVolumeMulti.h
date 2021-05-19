// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <numeric>
#include <vector>
// openvkl
#include "ProceduralVolumeMulti.h"
#include "TestingVolume.h"

namespace openvkl {
  namespace testing {

    struct TestingStructuredVolumeMulti : public TestingVolume,
                                          public ProceduralVolumeMulti
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

      float computeProceduralValueImpl(const vec3f &objectCoordinates,
                                       unsigned int attributeIndex,
                                       float time) const override;

      vec3f computeProceduralGradientImpl(const vec3f &objectCoordinates,
                                          unsigned int attributeIndex,
                                          float time) const override;

      std::string getGridType() const;
      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;
      TemporalConfig getTemporalConfig() const;

      unsigned int getNumAttributes() const override;

      vec3f transformLocalToObjectCoordinates(const vec3f &localCoordinates);

     protected:
      void generateVKLVolume(VKLDevice device) override final;

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
      std::vector<float> time;
      std::vector<uint32_t> tuvIndex;
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
          useAOSLayout(useAOSLayout),
          ProceduralVolumeMulti(temporalConfig.hasTime())
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
                          temporalConfig.isCompatible(v->getTemporalConfig());

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

    inline float TestingStructuredVolumeMulti::computeProceduralValueImpl(
        const vec3f &objectCoordinates,
        unsigned int attributeIndex,
        float time) const
    {
      return attributeVolumes[attributeIndex]->computeProceduralValue(
          objectCoordinates, time);
    }

    inline vec3f TestingStructuredVolumeMulti::computeProceduralGradientImpl(
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

    inline unsigned int TestingStructuredVolumeMulti::getNumAttributes() const
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

    inline void TestingStructuredVolumeMulti::generateVKLVolume(
        VKLDevice device)
    {
      volume = vklNewVolume(device, gridType.c_str());

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

        std::vector<unsigned char> av;

        // for each attribute: populate data into combined AOS voxel buffer, and
        // create (strided) VKL data object
        for (size_t i = 0; i < attributeVolumes.size(); i++) {
          const size_t voxelSizeOffset =
              std::accumulate(voxelSizes.begin(), voxelSizes.begin() + i, 0);

          av.clear();
          std::vector<float> at;
          std::vector<uint32_t> ai;
          attributeVolumes[i]->generateVoxels(av, at, ai);

          assert(at.empty());  // We do not have time in this branch.
          assert(ai.empty());
          assert(av.size() == dimensions.long_product() * voxelSizes[i]);

          for (size_t j = 0; j < dimensions.long_product(); j++) {
            std::memcpy(&v[j * voxelSizeSum + voxelSizeOffset],
                        &av[j * voxelSizes[i]],
                        voxelSizes[i]);
          }

          VKLData attributeData =
              vklNewData(device,
                         dimensions.long_product(),
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
          attributeVolumes[i]->generateVoxels(voxels[i], time, tuvIndex);

          assert(voxels[i].size() ==
                 dimensions.long_product() * temporalConfig.getNumSamples() *
                     sizeOfVKLDataType(attributeVolumes[i]->getVoxelType()));

          VKLData attributeData = vklNewData(
              device,
              dimensions.long_product() *
                  attributeVolumes[i]->getTemporalConfig().getNumSamples(),
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

      VKLData data = vklNewData(
          device, attributesData.size(), VKL_DATA, attributesData.data());
      for (const auto &d : attributesData) {
        vklRelease(d);
      }
      vklSetData(volume, "data", data);
      vklRelease(data);

      switch (temporalConfig.type) {
      case TemporalConfig::Constant: {
        break;
      }
      case TemporalConfig::Structured: {
        vklSetInt(volume,
                  "temporallyStructuredNumTimesteps",
                  temporalConfig.sampleTime.size());
        break;
      }

      case TemporalConfig::Unstructured: {
        VKLData indexData = vklNewData(device,
                                       tuvIndex.size(),
                                       VKL_UINT,
                                       tuvIndex.data(),
                                       dataCreationFlags);
        vklSetData(volume, "temporallyUnstructuredIndices", indexData);
        vklRelease(indexData);

        VKLData timeData = vklNewData(
            device, time.size(), VKL_FLOAT, time.data(), dataCreationFlags);
        vklSetData(volume, "temporallyUnstructuredTimes", timeData);
        vklRelease(timeData);
        break;
      }
      }

      vklCommit(volume);

      if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
        std::vector<std::vector<unsigned char>>().swap(voxels);
        std::vector<float>().swap(time);
        std::vector<uint32_t>().swap(tuvIndex);
      }

      // value range computation occurs during volume generation; we'll rely on
      // the individual attribute volumes for this, so we need to make sure
      // those volumes are generated.
      for (const auto &av : attributeVolumes) {
        av->getVKLVolume(device);
      }
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
