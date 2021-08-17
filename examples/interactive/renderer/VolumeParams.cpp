// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VolumeParams.h"
#include "CommandLine.h"

#include <rkcommon/os/library.h>  // make_unique

#include <algorithm>
#include <cassert>
#include <functional>
#include <type_traits>

// NOTE: Cannot use unordered_map with VKLDataType since libstdc++ has a bug
// on Centos 7/ICC15, and this prevents us from implementing a hash functor.
#include <map>

using namespace openvkl::testing;

namespace openvkl {
  namespace examples {

    // Note: Will only work for three letter extensions.
    inline std::string getFileExtension(const std::string &filename)
    {
      std::string ext;
      if (!filename.empty()) {
        ext = filename.substr(filename.size() - 4);
        std::for_each(
            ext.begin(), ext.end(), [](char &c) { c = ::tolower(c); });
      }
      return ext;
    }

    void VolumeParams::usage()
    {
      std::cerr
          << "\t-volumeType structuredRegular | structuredSpherical | "
             "unstructured | amr | vdb | particle\n"
             "\t-gridOrigin <x> <y> <z>\n"
             "\t-gridSpacing <x> <y> <z>\n"
             "\t-gridDimensions <dimX> <dimY> <dimZ>\n"
             "\t-voxelType uchar | short | ushort | half | float | double\n"
             "\t-multiAttribute (vdb and structuredRegular only, ignores "
             "-field)\n"
             "\t-motionBlur structured | unstructured (structuredRegular and "
             "vdb)\n"
             "\t-field wavelet | xyz | sphere | torus (vdb float only) | "
             "<FIELD> (-file only)\n"
             "\t-file <filename>\n"
             "\t-numParticles <N> (particle only)\n"
             "\t-background <BG VALUE>| undefined\n";
    }

    void VolumeParams::parseCommandLine(std::list<std::string> &args)
    {
      std::string fieldParam;
      for (auto it = args.begin(); it != args.end();) {
        const std::string arg = *it;

        if (arg == "-volumeType") {
          volumeType = cmd::consume_1<std::string>(args, it);
        }

        else if (arg == "-file") {
          filename = cmd::consume_1<std::string>(args, it);
          source = "file";
        }

        else if (arg == "-field") {
          fieldParam = cmd::consume_1<std::string>(args, it);
        }

        else if (arg == "-voxelType") {
          const auto vt = cmd::consume_1<std::string>(args, it);
          voxelType     = stringToVoxelType(vt);
        }

        else if (arg == "-background") {
          try {
            background = cmd::consume_1<float>(args, it);
          } catch (std::runtime_error) {
            const std::string bgStr = cmd::consume_1<std::string>(args, it);
            if (bgStr == "undefined")
              background = VKL_BACKGROUND_UNDEFINED;
            else
              throw std::runtime_error("-background: invalid argument");
          }
        }

        else if (arg == "-numParticles") {
          numParticles = cmd::consume_1<int>(args, it);
        }

        else if (arg == "-gridOrigin") {
          std::tie(gridOrigin.x, gridOrigin.y, gridOrigin.z) =
              cmd::consume_3<float, float, float>(args, it);
        }

        else if (arg == "-gridSpacing") {
          std::tie(gridSpacing.x, gridSpacing.y, gridSpacing.z) =
              cmd::consume_3<float, float, float>(args, it);
        }

        else if (arg == "-gridDimensions") {
          std::tie(dimensions.x, dimensions.y, dimensions.z) =
              cmd::consume_3<int, int, int>(args, it);
        }

        else if (arg == "-multiAttribute") {
          cmd::consume_0(args, it);
          multiAttribute = true;
        }

        else if (arg == "-motionBlur") {
          const auto mbType = cmd::consume_1<std::string>(args, it);
          if (mbType == "structured")
            motionBlurStructured = true;
          else if (mbType == "unstructured")
            motionBlurUnstructured = true;
          else
            throw std::runtime_error("unknown motion blur type " + mbType);
        }

        else {
          ++it;
        }
      }

      if (!fieldParam.empty()) {
        if (filename.empty()) {
          field  = fieldParam;
          source = "procedural";
        } else {
          fieldInFile = fieldParam;
          source      = "file";
        }
      }

      generateGridTransform();
      validate();
    }

    void VolumeParams::generateGridTransform()
    {
      // generate gridOrigin and gridSpacing if not specified on
      // the command-line
      if (std::isnan(gridOrigin.x) || std::isnan(gridSpacing.x)) {
        const float boundingBoxSize = 2.f;

        if (volumeType == "structuredSpherical") {
          ProceduralStructuredSphericalVolume<>::generateGridParameters(
              dimensions, boundingBoxSize, gridOrigin, gridSpacing);
        } else {
          // all other grid types can use values generated for structured
          // regular volumes
          ProceduralStructuredRegularVolume<>::generateGridParameters(
              dimensions, boundingBoxSize, gridOrigin, gridSpacing);
        }
      }
    }

    void VolumeParams::validate() const
    {
      const std::vector<std::string> &supVt = supportedVolumeTypes();
      if (std::find(supVt.begin(), supVt.end(), volumeType) == supVt.end()) {
        throw std::runtime_error("invalid volume type " + volumeType);
      }

      const std::vector<std::string> &supSources = supportedSources(volumeType);
      if (std::find(supSources.begin(), supSources.end(), source) ==
          supSources.end()) {
        throw std::runtime_error("invalid source " + source);
      }

      if (filename.empty()) {
        const std::vector<std::string> &supFields = supportedFields(volumeType);
        if (std::find(supFields.begin(), supFields.end(), field) ==
            supFields.end()) {
          throw std::runtime_error("invalid field " + field);
        }
      }

      const std::vector<VKLDataType> &supVoxelTypes =
          supportedVoxelTypes(volumeType);
      if (!supVoxelTypes.empty() &&
          std::find(supVoxelTypes.begin(), supVoxelTypes.end(), voxelType) ==
              supVoxelTypes.end()) {
        throw std::runtime_error("invalid voxelType " +
                                 voxelTypeToString(voxelType));
      }
    }

    std::unique_ptr<testing::TestingVolume> VolumeParams::createVolume() const
    {
      // We make a copy so that we can fix things automagically without
      // having to make this function non-const.
      VolumeParams pp = *this;
      pp.validate();

      std::unique_ptr<testing::TestingVolume> testingVolume;

      if (volumeType == "structuredRegular") {
        testingVolume = pp.createStructuredRegularVolume();
      } else if (volumeType == "structuredSpherical") {
        testingVolume = pp.createStructuredSphericalVolume();
      } else if (volumeType == "unstructured") {
        testingVolume = pp.createUnstructuredVolume();
      } else if (volumeType == "amr") {
        testingVolume = pp.createAmrVolume();
      } else if (volumeType == "vdb") {
        testingVolume = pp.createVdbVolume();
      } else if (volumeType == "particle") {
        testingVolume = pp.createParticleVolume();
      }

      // We checked the volume type in validate()!
      assert(testingVolume);

      VKLVolume vklVolume = testingVolume->getVKLVolume(getOpenVKLDevice());
      vklSetFloat(vklVolume, "background", pp.background);
      vklCommit(vklVolume);

      return testingVolume;
    }

    std::unique_ptr<testing::TestingVolume>
    VolumeParams::createStructuredRegularVolume() const
    {
      if (source == "file") {
        const std::string ext = getFileExtension(filename);
        if (ext == ".rwh") {
          return rkcommon::make_unique<RawHFileStructuredVolume>(
              filename, volumeType, gridOrigin, gridSpacing);
        } else {
          return rkcommon::make_unique<RawFileStructuredVolume>(filename,
                                                                volumeType,
                                                                dimensions,
                                                                gridOrigin,
                                                                gridSpacing,
                                                                voxelType);
        }
      }

      // Source is not a file.
      const TemporalConfig temporalConfig = createTemporalConfig();

      if (multiAttribute) {
        return std::unique_ptr<TestingStructuredVolumeMulti>(
            generateMultiAttributeStructuredRegularVolume(
                dimensions,
                gridOrigin,
                gridSpacing,
                temporalConfig,
                VKL_DATA_SHARED_BUFFER,
                false));
      }

      //

      if (voxelType == VKL_UCHAR) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredRegularVolumeUChar>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredRegularVolumeUChar>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredRegularVolumeUChar>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }
      }  // VKL_UCHAR

      if (voxelType == VKL_SHORT) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredRegularVolumeShort>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredRegularVolumeShort>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredRegularVolumeShort>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }
      }  // VKL_SHORT

      if (voxelType == VKL_USHORT) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredRegularVolumeUShort>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredRegularVolumeUShort>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredRegularVolumeUShort>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }
      }  // VKL_USHORT

      if (voxelType == VKL_HALF) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredRegularVolumeHalf>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredRegularVolumeHalf>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredRegularVolumeHalf>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }
      }  // VKL_HALF

      if (voxelType == VKL_FLOAT) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredRegularVolumeFloat>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredRegularVolumeFloat>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredRegularVolumeFloat>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }
      }  // VKL_FLOAT

      if (voxelType == VKL_DOUBLE) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredRegularVolumeDouble>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredRegularVolumeDouble>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredRegularVolumeDouble>(
              dimensions, gridOrigin, gridSpacing, temporalConfig);
        }
      }  // VKL_DOUBLE

      // Since we validate input, this should never happen except in case
      // of a programmer error.
      assert(false);
      return nullptr;
    }

    std::unique_ptr<testing::TestingVolume>
    VolumeParams::createStructuredSphericalVolume() const
    {
      if (voxelType == VKL_UCHAR) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredSphericalVolumeUChar>(
              dimensions, gridOrigin, gridSpacing);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredSphericalVolumeUChar>(
              dimensions, gridOrigin, gridSpacing);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredSphericalVolumeUChar>(
              dimensions, gridOrigin, gridSpacing);
        }
      }  // VKL_UCHAR

      if (voxelType == VKL_SHORT) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredSphericalVolumeShort>(
              dimensions, gridOrigin, gridSpacing);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredSphericalVolumeShort>(
              dimensions, gridOrigin, gridSpacing);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredSphericalVolumeShort>(
              dimensions, gridOrigin, gridSpacing);
        }
      }  // VKL_SHORT

      if (voxelType == VKL_USHORT) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredSphericalVolumeUShort>(
              dimensions, gridOrigin, gridSpacing);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredSphericalVolumeUShort>(
              dimensions, gridOrigin, gridSpacing);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredSphericalVolumeUShort>(
              dimensions, gridOrigin, gridSpacing);
        }
      }  // VKL_USHORT

      if (voxelType == VKL_HALF) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredSphericalVolumeHalf>(
              dimensions, gridOrigin, gridSpacing);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredSphericalVolumeHalf>(
              dimensions, gridOrigin, gridSpacing);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredSphericalVolumeHalf>(
              dimensions, gridOrigin, gridSpacing);
        }
      }  // VKL_HALF

      if (voxelType == VKL_FLOAT) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredSphericalVolumeFloat>(
              dimensions, gridOrigin, gridSpacing);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredSphericalVolumeFloat>(
              dimensions, gridOrigin, gridSpacing);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredSphericalVolumeFloat>(
              dimensions, gridOrigin, gridSpacing);
        }
      }  // VKL_FLOAT

      if (voxelType == VKL_DOUBLE) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZStructuredSphericalVolumeDouble>(
              dimensions, gridOrigin, gridSpacing);
        }

        else if (field == "sphere") {
          return rkcommon::make_unique<SphereStructuredSphericalVolumeDouble>(
              dimensions, gridOrigin, gridSpacing);
        }

        else {
          return rkcommon::make_unique<WaveletStructuredSphericalVolumeDouble>(
              dimensions, gridOrigin, gridSpacing);
        }
      }  // VKL_DOUBLE

      // Since we validate input, this should never happen except in case
      // of a programmer error.
      assert(false);
      return nullptr;
    }

    std::unique_ptr<testing::TestingVolume>
    VolumeParams::createUnstructuredVolume() const
    {
      if (field == "xyz") {
        return rkcommon::make_unique<XYZUnstructuredProceduralVolume>(
            dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false);
      } else if (field == "sphere") {
        return rkcommon::make_unique<SphereUnstructuredProceduralVolume>(
            dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false);
      } else if (field == "mixed") {
        return rkcommon::make_unique<UnstructuredVolumeMixedSimple>();
      } else {
        return rkcommon::make_unique<WaveletUnstructuredProceduralVolume>(
            dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false);
      }

      // Since we validate input, this should never happen except in case
      // of a programmer error.
      assert(false);
      return nullptr;
    }

    std::unique_ptr<testing::TestingVolume> VolumeParams::createAmrVolume()
        const
    {
      return rkcommon::make_unique<ProceduralShellsAMRVolume<>>(
          dimensions, gridOrigin, gridSpacing);
    }

    std::unique_ptr<testing::TestingVolume> VolumeParams::createVdbVolume()
        const
    {
      if (source == "file") {
        // avoid deferred loading when exporting innerNodes to ensure exported
        // value ranges represent the full data
        return std::unique_ptr<OpenVdbVolume>(
            OpenVdbVolume::loadVdbFile(getOpenVKLDevice(),
                                       filename,
                                       fieldInFile,
                                       false /* do not defer leaves. */));
      }

      const TemporalConfig temporalConfig = createTemporalConfig();

      if (!temporalConfig.hasTime() && multiAttribute) {
        if (voxelType == VKL_HALF) {
          return std::unique_ptr<ProceduralVdbVolumeMulti>(
              generateMultiAttributeVdbVolumeHalf(getOpenVKLDevice(),
                                                  dimensions,
                                                  gridOrigin,
                                                  gridSpacing,
                                                  VKL_DATA_SHARED_BUFFER,
                                                  false));
        } else if (voxelType == VKL_FLOAT) {
          return std::unique_ptr<ProceduralVdbVolumeMulti>(
              generateMultiAttributeVdbVolumeFloat(getOpenVKLDevice(),
                                                   dimensions,
                                                   gridOrigin,
                                                   gridSpacing,
                                                   VKL_DATA_SHARED_BUFFER,
                                                   false));
        }
      }

      const uint32_t numAttributes = multiAttribute ? 3 : 1;

      if (voxelType == VKL_HALF) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZVdbVolumeHalf>(getOpenVKLDevice(),
                                                         dimensions,
                                                         gridOrigin,
                                                         gridSpacing,
                                                         temporalConfig,
                                                         numAttributes);
        } else if (field == "sphere") {
          return rkcommon::make_unique<SphereVdbVolumeHalf>(getOpenVKLDevice(),
                                                            dimensions,
                                                            gridOrigin,
                                                            gridSpacing,
                                                            temporalConfig,
                                                            numAttributes);
        } else {
          return rkcommon::make_unique<WaveletVdbVolumeHalf>(getOpenVKLDevice(),
                                                             dimensions,
                                                             gridOrigin,
                                                             gridSpacing,
                                                             temporalConfig,
                                                             numAttributes);
        }
      }  // VKL_HALF

      if (voxelType == VKL_FLOAT) {
        if (field == "xyz") {
          return rkcommon::make_unique<XYZVdbVolumeFloat>(getOpenVKLDevice(),
                                                          dimensions,
                                                          gridOrigin,
                                                          gridSpacing,
                                                          temporalConfig,
                                                          numAttributes);
        } else if (field == "sphere") {
          return rkcommon::make_unique<SphereVdbVolumeFloat>(getOpenVKLDevice(),
                                                             dimensions,
                                                             gridOrigin,
                                                             gridSpacing,
                                                             temporalConfig,
                                                             numAttributes);
        } else {
          return rkcommon::make_unique<WaveletVdbVolumeFloat>(
              getOpenVKLDevice(),
              dimensions,
              gridOrigin,
              gridSpacing,
              temporalConfig,
              numAttributes);
        }
      }  // VKL_FLOAT

      // Since we validate input, this should never happen except in case
      // of a programmer error.
      assert(false);
      return nullptr;
    }

    std::unique_ptr<testing::TestingVolume> VolumeParams::createParticleVolume()
        const
    {
      return rkcommon::make_unique<ProceduralParticleVolume>(numParticles);
    }

    TemporalConfig VolumeParams::createTemporalConfig() const
    {
      TemporalConfig temporalConfig;

      if (motionBlurStructured) {
        temporalConfig = TemporalConfig(TemporalConfig::Structured,
                                        motionBlurStructuredNumTimesteps);
      } else if (motionBlurUnstructured) {
        if (field == "sphere" && !multiAttribute) {
          temporalConfig = TemporalConfig(TemporalConfig::Unstructured, 256);
          temporalConfig.useTemporalCompression       = true;
          temporalConfig.temporalCompressionThreshold = 0.05f;
        } else {
          std::vector<float> timeSamples = motionBlurUnstructuredTimeSamples;
          std::sort(timeSamples.begin(), timeSamples.end());
          temporalConfig = TemporalConfig(timeSamples);
        }
      }

      return temporalConfig;
    }

  }  // namespace examples
}  // namespace openvkl
