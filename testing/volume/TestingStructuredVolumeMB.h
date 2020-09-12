// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
// openvkl
#include "TestingVolume.h"
// rkcommon
#include "rkcommon/math/range.h"

namespace openvkl {
  namespace testing {

    struct TestingTemporalData 
    {
      TestingTemporalData (unsigned int numTimeSteps, std::vector<float> timeSamples)
      : 
        numTimeSteps (numTimeSteps)
      , timeSamples (timeSamples)
      {};

      unsigned int numTimeSteps;
      std::vector<float> timeSamples;
    };

    struct TestingStructuredVolumeMB : public TestingVolume
    {
      TestingStructuredVolumeMB(
          const std::string &gridType,
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          const std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>>
              &attributeVolumes,
          const std::vector<TestingTemporalData> &attrTimeData,
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT);

      // maps to first attribute only
      range1f getComputedValueRange() const override;

      range1f getComputedValueRange(unsigned int attributeIndex) const;

      float computeProceduralValue(const vec3f &objectCoordinates,
                                   unsigned int attributeIndex) const;
      
      float computeProceduralValue(const vec3f &objectCoordinates,
                                   float time,
                                   unsigned int attributeIndex) const;

      vec3f computeProceduralGradient(const vec3f &objectCoordinates,
                                      unsigned int attributeIndex) const;

      std::string getGridType() const;
      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;

      unsigned int getNumAttributes();

      vec3f transformLocalToObjectCoordinates(const vec3f &localCoordinates);

     protected:
      void generateVKLVolume() override;

      range1f computedValueRange = range1f(rkcommon::math::empty);

      std::string gridType;
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      VKLDataCreationFlags dataCreationFlags;

      std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>>
          attributeVolumes;
      std::vector<TestingTemporalData>        attrTimeData;
      std::vector<std::vector<unsigned int>>  timeConfiguration; 
      std::vector<std::vector<float>>         timeSamples;
      std::vector<std::vector<unsigned char>> voxels;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline TestingStructuredVolumeMB::TestingStructuredVolumeMB(
        const std::string &gridType,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        const std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>>
            &attributeVolumes,
        const std::vector<TestingTemporalData> &attrTimeData,
        VKLDataCreationFlags dataCreationFlags)
        : gridType(gridType),
          dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing),
          attributeVolumes(attributeVolumes),
          attrTimeData(attrTimeData),
          dataCreationFlags(dataCreationFlags)
    {
      if (attributeVolumes.size() == 0) {
        throw std::runtime_error("no provided attribute volumes");
      }

      if (attributeVolumes.size() != attrTimeData.size()) {
        throw std::runtime_error(
            "attribute volumes and attribute time data sizes don't match");
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
              "constructed TestingStructuredVolumeMB instance");
        }
      }
    }

    inline range1f TestingStructuredVolumeMB::getComputedValueRange() const
    {
      return attributeVolumes[0]->getComputedValueRange();
    }

    inline range1f TestingStructuredVolumeMB::getComputedValueRange(
        unsigned int attributeIndex) const
    {
      return attributeVolumes[attributeIndex]->getComputedValueRange();
    }

    inline float TestingStructuredVolumeMB::computeProceduralValue(
        const vec3f &objectCoordinates, unsigned int attributeIndex) const
    {
      return attributeVolumes[attributeIndex]->computeProceduralValue(
          objectCoordinates);
    }

    inline float TestingStructuredVolumeMB::computeProceduralValue(
        const vec3f &objectCoordinates, float time, unsigned int attributeIndex) const
    {
      return attributeVolumes[attributeIndex]->computeProceduralValueMB(
          objectCoordinates, time);
    }

    inline vec3f TestingStructuredVolumeMB::computeProceduralGradient(
        const vec3f &objectCoordinates, unsigned int attributeIndex) const
    {
      return attributeVolumes[attributeIndex]->computeProceduralGradient(
          objectCoordinates);
    }

    inline std::string TestingStructuredVolumeMB::getGridType() const
    {
      return gridType;
    }

    inline vec3i TestingStructuredVolumeMB::getDimensions() const
    {
      return dimensions;
    }

    inline vec3f TestingStructuredVolumeMB::getGridOrigin() const
    {
      return gridOrigin;
    }

    inline vec3f TestingStructuredVolumeMB::getGridSpacing() const
    {
      return gridSpacing;
    }

    inline unsigned int TestingStructuredVolumeMB::getNumAttributes()
    {
      return attributeVolumes.size();
    }

    inline vec3f
    TestingStructuredVolumeMB::transformLocalToObjectCoordinates(
        const vec3f &localCoordinates)
    {
      // at construction we're guaranteed all attribute volumes have the same
      // grid parameters, so we can simply do the transformation on the first
      // volume
      return attributeVolumes[0]->transformLocalToObjectCoordinates(
          localCoordinates);
    }

    inline void TestingStructuredVolumeMB::generateVKLVolume()
    {
      volume = vklNewVolume(gridType.c_str());

      vklSetVec3i(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetVec3f(
          volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vklSetVec3f(
          volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

      voxels.resize(attributeVolumes.size());
      timeSamples.resize(attributeVolumes.size());
      timeConfiguration.resize(attributeVolumes.size());

      std::vector<VKLData> attributesData;
      std::vector<VKLData> attributesTimeData;        
      std::vector<VKLData> attributesTimeConfig;

      for (int i = 0; i < attributeVolumes.size(); i++) {
          voxels[i] = 
              attributeVolumes[i]->generateVoxels(attrTimeData[i].numTimeSteps,
                                                  attrTimeData[i].timeSamples);
          timeSamples[i] = 
              attributeVolumes[i]->generateTimeData(attrTimeData[i].numTimeSteps,
                                                    attrTimeData[i].timeSamples);
          timeConfiguration[i] = 
              attributeVolumes[i]->generateTimeConfig(attrTimeData[i].numTimeSteps,
                                                      attrTimeData[i].timeSamples);

          VKLData attributeData =
              vklNewData(dimensions.long_product()*attrTimeData[i].numTimeSteps,
                         attributeVolumes[i]->getVoxelType(),
                         voxels[i].data(),
                         dataCreationFlags,
                         0);
          attributesData.push_back(attributeData);
          VKLData attributeTimeConfig =
              vklNewData(timeConfiguration[i].size(),
                         VKL_UINT,
                         timeConfiguration[i].data(),
                         dataCreationFlags,
                         0);
          attributesTimeConfig.push_back(attributeTimeConfig);

          if (timeSamples[i].size() == 0) {
            attributesTimeData.push_back(nullptr);
          } else {
            VKLData attributeTimeData =
                vklNewData(timeSamples[i].size(),
                          VKL_FLOAT,
                          timeSamples[i].data(),
                          dataCreationFlags,
                          0);
            attributesTimeData.push_back(attributeTimeData);
          }

          if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
            std::vector<unsigned char>().swap(voxels[i]);
            std::vector<float>().swap(timeSamples[i]);
            std::vector<unsigned int>().swap(timeConfiguration[i]);
          }
        }


      VKLData data =
          vklNewData(attributesData.size(), VKL_DATA, attributesData.data());
      VKLData timeData =
          vklNewData(attributesTimeData.size(), VKL_DATA, attributesTimeData.data());
      VKLData timeConfig =
          vklNewData(attributesTimeConfig.size(), VKL_DATA, attributesTimeConfig.data());

      for (const auto &d : attributesData) {
        vklRelease(d);
      }
      for (const auto &d : attributesTimeData) {
        if (d) {
          vklRelease(d);
        }
      }
      for (const auto &d : attributesTimeConfig) {
        vklRelease(d);
      }

      vklSetData(volume, "data", data);
      vklSetData(volume, "timeData", timeData);
      vklSetData(volume, "timeConfig", timeConfig);
      
      vklRelease(data);
      vklRelease(timeData);
      vklRelease(timeConfig);

      vklCommit(volume);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume generation helpers ///////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    inline TestingStructuredVolumeMB *
    generateMultiAttributeStructuredRegularVolumeMB(
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        VKLDataCreationFlags dataCreationFlags)
    {
      std::vector<std::shared_ptr<ProceduralStructuredVolumeBase>> volumes;
      std::vector<TestingTemporalData> timeData;

      volumes.push_back(std::make_shared<WaveletStructuredRegularVolumeFloat>(
          dimensions, gridOrigin, gridSpacing));
      timeData.push_back(TestingTemporalData(1,std::vector<float>()));

      std::vector<float> timeSamples {0.f, 0.15f, 0.3f, 0.65f, 0.9f, 1.0f};
      volumes.push_back(std::make_shared<XYZStructuredRegularVolume<float>>(
          dimensions, gridOrigin, gridSpacing));
      timeData.push_back(TestingTemporalData(timeSamples.size(), timeSamples));

      volumes.push_back(std::make_shared<XYZStructuredRegularVolume<float>>(
          dimensions, gridOrigin, gridSpacing));
      timeData.push_back(TestingTemporalData(3,std::vector<float>()));

      return new TestingStructuredVolumeMB("structuredRegular",
                                              dimensions,
                                              gridOrigin,
                                              gridSpacing,
                                              volumes,
                                              timeData,
                                              dataCreationFlags);
    }

  }  // namespace testing
}  // namespace openvkl
