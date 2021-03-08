// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TestingStructuredVolume.h"
// std
#include <algorithm>
#include <fstream>

using namespace rkcommon;

namespace openvkl {
  namespace testing {

    struct RawHFileStructuredVolume final : public TestingStructuredVolume
    {
      RawHFileStructuredVolume(const std::string &filename,
                               const std::string &gridType,
                               const vec3f &gridOrigin,
                               const vec3f &gridSpacing);

      void generateVoxels(std::vector<unsigned char> &voxels,
                          std::vector<float> &time,
                          std::vector<uint32_t> &tuvIndex) const override final;

     private:
      void readStaticFile(std::ifstream &input,
                          std::vector<unsigned char> &voxels,
                          std::vector<float> &time,
                          std::vector<uint32_t> &tuvIndex) const;
      void readStructuredMBFile(std::ifstream &input,
                                std::vector<unsigned char> &voxels,
                                std::vector<float> &time,
                                std::vector<uint32_t> &tuvIndex) const;
      void readUnstructuredMBFile(std::ifstream &input,
                                  std::vector<unsigned char> &voxels,
                                  std::vector<float> &time,
                                  std::vector<uint32_t> &tuvIndex) const;

      std::string filename;
      size_t readPosition{0};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline RawHFileStructuredVolume::RawHFileStructuredVolume(
        const std::string &filename,
        const std::string &gridType,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing)
        : filename(filename),
          TestingStructuredVolume(
              gridType,
              vec3i(0) /* will be ready from file */,
              gridOrigin,
              gridSpacing,
              TemporalConfig(),
              VKL_FLOAT /* currently format is limited to float only */)
    {
      std::ifstream input(filename.c_str(), std::ios::binary);
      if (!input) {
        throw std::runtime_error("error opening rawh volume file");
      }

      int configType;
      input.read((char *)(&configType), sizeof(int));
      if (!input.good()) {
        throw std::runtime_error(
            "error reading rawh volume file - temporal data type");
      }

      int buffer[3];
      input.read((char *)(&buffer[0]), 3 * sizeof(int));
      if (!input.good()) {
        throw std::runtime_error(
            "error reading rawh structured mb file - dimensions");
      }

      this->dimensions   = vec3i(buffer[0], buffer[1], buffer[2]);
      this->readPosition = 4 * sizeof(int);

      switch (configType) {
      case 0 /*static (no motion blur)*/:
        break;
      case 1 /*structured motion blur*/:
        size_t timeStepBuffer;
        input.read((char *)(&timeStepBuffer), sizeof(size_t));
        if (!input.good()) {
          throw std::runtime_error(
              "error reading rawh structured mb file - nTimeSteps");
        }
        this->readPosition += sizeof(size_t);
        temporalConfig =
            TemporalConfig(TemporalConfig::Type::Structured, timeStepBuffer);
        break;
      case 2 /*unstructured motion blur*/:
        this->temporalConfig.type = TemporalConfig::Type::Unstructured;
        break;
      default:
        throw std::runtime_error(
            "error reading rawh mb file - illegal configType");
      }
    }

    inline void RawHFileStructuredVolume::readStaticFile(
        std::ifstream &input,
        std::vector<unsigned char> &voxels,
        std::vector<float> &time,
        std::vector<uint32_t> &tuvIndex) const
    {
      const size_t numValues = dimensions.long_product();
      const size_t numBytes  = numValues * sizeOfVKLDataType(voxelType);

      voxels.resize(numBytes);
      time.clear();
      tuvIndex.clear();

      input.read((char *)voxels.data(), numBytes);
      if (!input.good()) {
        throw std::runtime_error("error reading raw volume file");
      }
    }

    inline void RawHFileStructuredVolume::readStructuredMBFile(
        std::ifstream &input,
        std::vector<unsigned char> &voxels,
        std::vector<float> &time,
        std::vector<uint32_t> &tuvIndex) const
    {
      const size_t numValues =
          dimensions.long_product() * temporalConfig.sampleTime.size();
      const size_t numBytes = numValues * sizeOfVKLDataType(voxelType);

      voxels.resize(numBytes);
      time.clear();
      tuvIndex.clear();

      input.read((char *)voxels.data(), numBytes);
      if (!input.good()) {
        throw std::runtime_error("error reading raw volume file");
      }
    }

    inline void RawHFileStructuredVolume::readUnstructuredMBFile(
        std::ifstream &input,
        std::vector<unsigned char> &voxels,
        std::vector<float> &time,
        std::vector<uint32_t> &tuvIndex) const
    {
      const size_t numIndices = dimensions.long_product() + 1;

      tuvIndex.resize(numIndices);
      input.read((char *)tuvIndex.data(), numIndices * sizeof(uint32_t));
      if (!input.good()) {
        throw std::runtime_error("error reading unstructured volume time data");
      }

      const size_t numValues = *tuvIndex.rbegin();
      const size_t numBytes  = numValues * sizeof(float);

      time.resize(numValues);
      input.read((char *)time.data(), numValues * sizeof(float));
      if (!input.good()) {
        throw std::runtime_error("error reading unstructured volume time data");
      }

      voxels.resize(numBytes);
      input.read((char *)voxels.data(),
                 numValues * sizeOfVKLDataType(voxelType));
      if (!input.good()) {
        throw std::runtime_error(
            "error reading unstructured volume voxel data");
      }
    }

    inline void RawHFileStructuredVolume::generateVoxels(
        std::vector<unsigned char> &voxels,
        std::vector<float> &time,
        std::vector<uint32_t> &tuvIndex) const
    {
      std::ifstream input(filename.c_str(), std::ios::binary);
      if (!input) {
        throw std::runtime_error(
            "StructuredRegular::render(): "
            "error opening mb volume file");
      }
      input.ignore(readPosition);
      if (!input.good()) {
        throw std::runtime_error(
            "error reading rawh structured mb file - dimensions");
      }

      switch (temporalConfig.type) {
      case TemporalConfig::Type::Constant:
        readStaticFile(input, voxels, time, tuvIndex);
        break;
      case TemporalConfig::Type::Structured:
        readStructuredMBFile(input, voxels, time, tuvIndex);
        break;
      case TemporalConfig::Type::Unstructured:
        readUnstructuredMBFile(input, voxels, time, tuvIndex);
        break;
      }
    }

  }  // namespace testing
}  // namespace openvkl
