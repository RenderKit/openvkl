// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <openvkl/openvkl.h>
#include "openvkl_testing.h"

#include <rkcommon/math/constants.h>
#include <rkcommon/math/vec.h>

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace openvkl {
  namespace examples {

    using namespace rkcommon::math;
    using namespace openvkl::testing;

    inline VKLDataType stringToVoxelType(const std::string &type)
    {
      static const std::map<std::string, VKLDataType> map = {
          {"uchar", VKL_UCHAR},
          {"short", VKL_SHORT},
          {"ushort", VKL_USHORT},
          {"half", VKL_HALF},
          {"float", VKL_FLOAT},
          {"double", VKL_DOUBLE}};

      const auto it = map.find(type);
      if (it == map.end()) {
        return VKL_UNKNOWN;
      }

      return it->second;
    }

    inline const std::string &voxelTypeToString(VKLDataType type)
    {
      static const std::map<VKLDataType, std::string> map = {
          {VKL_UCHAR, "uchar"},
          {VKL_SHORT, "short"},
          {VKL_USHORT, "ushort"},
          {VKL_HALF, "half"},
          {VKL_FLOAT, "float"},
          {VKL_DOUBLE, "double"}};

      const auto it = map.find(type);
      if (it == map.end()) {
        static const std::string uk = "unknown";
        return uk;
      }

      return it->second;
    }

    struct VolumeParams
    {
      std::string volumeType{supportedVolumeTypes()[0]};
      std::string source{supportedSources("structuredRegular")[0]};
      std::string filename;
      std::string field{supportedFields("structuredRegular")[0]};
      std::string fieldInFile{"density"};
      VKLDataType voxelType{VKL_FLOAT};
      float background{VKL_BACKGROUND_UNDEFINED};
      int numParticles{1000};

      vec3i dimensions{128};
      vec3f gridOrigin{rkcommon::math::nan};
      vec3f gridSpacing{rkcommon::math::nan};

      bool multiAttribute{false};

      bool motionBlurStructured{false};
      bool motionBlurUnstructured{false};
      std::vector<float> motionBlurUnstructuredTimeSamples{
          0.f, 0.15f, 0.3f, 0.65f, 0.9f, 1.0f};
      uint8_t motionBlurStructuredNumTimesteps{6};

      void parseCommandLine(std::list<std::string> &args);
      static void usage();

      // This method will throw if there is an error in creating the volume.
      std::unique_ptr<testing::TestingVolume> createVolume() const;

     private:
      void validate() const;
      TemporalConfig createTemporalConfig() const;

      std::unique_ptr<testing::TestingVolume> createStructuredRegularVolume()
          const;
      std::unique_ptr<testing::TestingVolume> createStructuredSphericalVolume()
          const;
      std::unique_ptr<testing::TestingVolume> createUnstructuredVolume() const;
      std::unique_ptr<testing::TestingVolume> createAmrVolume() const;
      std::unique_ptr<testing::TestingVolume> createVdbVolume() const;
      std::unique_ptr<testing::TestingVolume> createParticleVolume() const;

     public:
      void generateGridTransform();

      // Shared volume params. Only include things here that are valid for
      // all volume types, and should remain the same when switching volume
      // types.
      static const std::vector<std::string> &supportedVolumeTypes()
      {
        static std::vector<std::string> sup = {"structuredRegular",
                                               "structuredSpherical",
                                               "unstructured",
                                               "amr",
                                               "vdb",
                                               "particle"};
        return sup;
      };

      static const std::vector<VKLDataType> &supportedVoxelTypes(
          const std::string &volumeType)
      {
        if (volumeType == "structuredRegular" ||
            volumeType == "structuredSpherical") {
          static const std::vector<VKLDataType> types = {VKL_UCHAR,
                                                         VKL_SHORT,
                                                         VKL_USHORT,
                                                         VKL_HALF,
                                                         VKL_FLOAT,
                                                         VKL_DOUBLE};
          return types;
        }

        else if (volumeType == "vdb") {
          static const std::vector<VKLDataType> types = {VKL_HALF, VKL_FLOAT};
          return types;
        }

        static const std::vector<VKLDataType> types;
        return types;
      }

      static const std::vector<std::string> &supportedSources(
          const std::string &volumeType)
      {
        if (volumeType == "structuredRegular" || volumeType == "vdb") {
          static const std::vector<std::string> sources = {
              "procedural", "file"};
          return sources;
        }
        static const std::vector<std::string> sources = {
            "procedural"};
        return sources;
      };


      static const std::vector<std::string> &supportedFields(
          const std::string &volumeType)
      {
        if (volumeType == "unstructured") {
          static const std::vector<std::string> fields = {
              "wavelet", "xyz", "sphere", "mixed"};
          return fields;
        }

        static const std::vector<std::string> fields = {
            "wavelet", "xyz", "sphere"};
        return fields;
      };
    };

  }  // namespace examples
}  // namespace openvkl
