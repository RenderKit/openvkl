// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "window/GLFWVKLWindow.h"
#include "window/TransferFunctionWidget.h"
// openvkl_testing
#include "openvkl/utility/vdb/InnerNodes.h"
#include "openvkl_testing.h"
// imgui
#include <imgui.h>
// std
#include <array>
#include <map>
#include <memory>
#include <random>

using namespace rkcommon;
using namespace openvkl::testing;
using namespace openvkl::examples;

struct ViewerParams
{
  vec3f gridOrigin{rkcommon::nan};
  vec3f gridSpacing{rkcommon::nan};
  vec3i dimensions{128};
  vec2i windowSize{1024};
  region2i pixelRange{vec2i(0, 0), windowSize};
  std::string rendererType{"density_pathtracer"};
  std::string gridType{"structuredRegular"};
  std::string voxelTypeString{"float"};
  std::string filename;
  std::string fileNameOut;
  std::string field;
  std::vector<float> motionBlurUnstructuredTimeSamples{
      0.f, 0.15f, 0.3f, 0.65f, 0.9f, 1.0f};
  range1f initialValueRange{0.f, 1.f};
  VKLDataType voxelType{VKL_FLOAT};
  VKLFilter filter{VKL_FILTER_TRILINEAR};
  VKLFilter gradientFilter{VKL_FILTER_TRILINEAR};
  size_t numParticles{1000};
  int maxIteratorDepth{0};
  bool elementaryCellIteration{false};
  int maxSamplingDepth = VKL_VDB_NUM_LEVELS - 1;
  uint8_t motionBlurStructuredNumTimesteps{6};
  bool multiAttribute{false};
  bool motionBlurStructured{false};
  bool motionBlurUnstructured{false};
  bool haveFilter{false};
  bool haveVdb{false};
  bool disableVSync{false};
  bool interactive{true};
  bool useISPC{true};
  std::string innerNodeOutput;
  int innerNodeMaxDepth{1};
};

bool addSamplingRateUI(GLFWVKLWindow &window)
{
  auto &renderer = window.currentRenderer();

  static float samplingRate = 1.f;
  if (ImGui::SliderFloat("samplingRate", &samplingRate, 0.01f, 4.f)) {
    renderer.setParam<float>("samplingRate", samplingRate);
    renderer.commit();
    return true;
  }

  return false;
}

bool addPathTracerUI(GLFWVKLWindow &window, Scene &scene)
{
  auto &renderer = window.currentRenderer();

  bool changed = false;

  static int attributeIndex  = 0;
  unsigned int numAttributes = vklGetNumAttributes(scene.volume);

  if (numAttributes > 1) {
    if (ImGui::SliderInt(
            "attributeIndex", &attributeIndex, 0, numAttributes - 1)) {
      scene.attributeIndex = attributeIndex;
      changed              = true;
    }
  }

  static float time = 0.f;
  if (ImGui::SliderFloat("time", &time, 0.f, 1.f)) {
    renderer.setParam<float>("time", time);
    changed = true;
  }

  static bool motionBlur = false;
  if (ImGui::Checkbox("motion blur", &motionBlur)) {
    renderer.setParam<bool>("motionBlur", motionBlur);
    changed = true;
  }

  static float shutter = 0.f;
  if (motionBlur && ImGui::SliderFloat("shutter", &shutter, 0.f, 1.f)) {
    renderer.setParam<float>("shutter", shutter);
    changed = true;
  }

  static float sigmaTScale = 1.f;
  if (ImGui::SliderFloat("sigmaTScale", &sigmaTScale, 0.001f, 100.f)) {
    renderer.setParam<float>("sigmaTScale", sigmaTScale);
    changed = true;
  }

  static float sigmaSScale = 1.f;
  if (ImGui::SliderFloat("sigmaSScale", &sigmaSScale, 0.01f, 1.f)) {
    renderer.setParam<float>("sigmaSScale", sigmaSScale);
    changed = true;
  }

  static int maxNumScatters = 1;
  if (ImGui::SliderInt("maxNumScatters", &maxNumScatters, 1, 32)) {
    renderer.setParam<int>("maxNumScatters", maxNumScatters);
    changed = true;
  }

  static float ambientLightIntensity = 1.f;
  if (ImGui::SliderFloat(
          "ambientLightIntensity", &ambientLightIntensity, 0.f, 10.f)) {
    renderer.setParam<float>("ambientLightIntensity", ambientLightIntensity);
    changed = true;
  }

  if (changed) {
    renderer.commit();
  }

  return changed;
}

bool addIsosurfacesUI(GLFWVKLWindow &window, std::vector<float> &isoValues)
{
  auto &renderer = window.currentRenderer();

  static bool showIsosurfaces = true;

  static constexpr int maxNumIsosurfaces = 3;

  struct IsosurfaceParameters
  {
    bool enabled{true};
    float isovalue{0.f};
  };

  static std::array<IsosurfaceParameters, maxNumIsosurfaces> isosurfaces;

  static bool initialized = false;
  bool isosurfacesChanged = false;

  if (!initialized) {
    isosurfaces[0].isovalue = -1.f;
    isosurfaces[1].isovalue = 0.f;
    isosurfaces[2].isovalue = 1.f;

    initialized        = true;
    isosurfacesChanged = true;  // Update isovalues on init!
  }

  static float time = 0.f;
  if (ImGui::SliderFloat("time", &time, 0.f, 1.f)) {
    renderer.setParam<float>("time", time);
    isosurfacesChanged = true;
  }

  if (ImGui::Checkbox("show isosurfaces", &showIsosurfaces)) {
    isosurfacesChanged = true;
  }

  if (showIsosurfaces) {
    int labelCounter = 0;

    for (auto &isosurface : isosurfaces) {
      std::ostringstream enabledLabel;
      enabledLabel << "##enabled_isosurface " << labelCounter;

      std::ostringstream isovalueLabel;
      isovalueLabel << "isosurface " << labelCounter;

      if (ImGui::Checkbox(enabledLabel.str().c_str(), &isosurface.enabled)) {
        isosurfacesChanged = true;
      }

      ImGui::SameLine();

      if (ImGui::SliderFloat(
              isovalueLabel.str().c_str(), &isosurface.isovalue, -1.f, 1.f)) {
        isosurfacesChanged = true;
      }

      labelCounter++;
    }
  }

  if (isosurfacesChanged) {
    isoValues.clear();

    if (showIsosurfaces) {
      for (const auto &isosurface : isosurfaces) {
        if (isosurface.enabled) {
          isoValues.push_back(isosurface.isovalue);
        }
      }
    }
  }

  if (isosurfacesChanged) {
    renderer.commit();
  }

  return isosurfacesChanged;
}

bool addIntervalIteratorDebugUI(GLFWVKLWindow &window)
{
  auto &renderer = window.currentRenderer();

  bool changed = false;

  static int intervalColorScale = 4;
  if (ImGui::SliderInt("intervalColorScale", &intervalColorScale, 1, 32)) {
    renderer.setParam<float>("intervalColorScale", float(intervalColorScale));
    changed = true;
  }

  static float intervalOpacity = 0.25f;
  if (ImGui::SliderFloat("intervalOpacity", &intervalOpacity, 0.01f, 1.f)) {
    renderer.setParam<float>("intervalOpacity", intervalOpacity);
    changed = true;
  }

  static bool firstIntervalOnly = false;
  if (ImGui::Checkbox("firstIntervalOnly", &firstIntervalOnly)) {
    renderer.setParam<bool>("firstIntervalOnly", firstIntervalOnly);
    changed = true;
  }

  static bool showIntervalBorders = false;
  if (ImGui::Checkbox("showIntervalBorders", &showIntervalBorders)) {
    renderer.setParam<bool>("showIntervalBorders", showIntervalBorders);
    changed = true;
  }

  if (changed) {
    renderer.commit();
  }

  return changed;
}

void usage(const char *progname)
{
  std::cerr
      << "usage: " << progname << "\n"
      << "\t-renderer density_pathtracer | hit_iterator |"
         " ray_march_iterator | interval_iterator_debug\n"
         "\t-gridType structuredRegular | structuredSpherical | "
         "unstructured | amr | vdb | particle\n"
         "\t-gridOrigin <x> <y> <z>\n"
         "\t-gridSpacing <x> <y> <z>\n"
         "\t-gridDimensions <dimX> <dimY> <dimZ>\n"
         "\t-voxelType uchar | short | ushort | half | float | double\n"
         "\t-valueRange <lower> <upper>\n"
         "\t-multiAttribute (vdb and structuredRegular only, ignores -field)\n"
         "\t-motionBlur structured | unstructured (structuredRegular and vdb)\n"
         "\t-filter nearest | trilinear (structured and vdb) | tricubic (vdb)\n"
         "\t-field wavelet | xyz | sphere | <vdb grid name>\n"
         "\t-file <filename>\n"
         "\t-numParticles <N> (particle only)\n"
         "\t-disable-vsync\n"
         "\t-ispc\n"
         "\t-windowSize <x> <y>\n"
         "\t-pixelRange <xMin> <yMin> <xMax> <yMax>\n"
         "\t-o <output.ppm>\n"
         "\t-innerNodeOutput <output.usda> (vdb only)\n"
         "\t-innerNodeMaxDepth <level> (vdb only)\n"
      << std::endl;
}

bool parseCommandLine(int argc, const char **argv, ViewerParams &params)
{
  int argIndex = 1;
  while (argIndex < argc) {
    std::string switchArg(argv[argIndex++]);

    if (switchArg == "-gridType") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -gridType arguments");
      }

      params.gridType = std::string(argv[argIndex++]);
    } else if (switchArg == "-o") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -o arguments");
      }
      params.interactive = false;
      params.fileNameOut = std::string(argv[argIndex++]);
    } else if (switchArg == "-ispc") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -ispc arguments");
      }
      params.useISPC = atoi(argv[argIndex++]);
    } else if (switchArg == "-windowSize") {
      if (argc < argIndex + 2) {
        throw std::runtime_error("improper -windowSize arguments");
      }
      params.windowSize.x     = atoi(argv[argIndex++]);
      params.windowSize.y     = atoi(argv[argIndex++]);
      params.pixelRange.lower = vec2i(0);
      params.pixelRange.upper = params.windowSize;
    } else if (switchArg == "-pixelRange") {
      if (argc < argIndex + 4) {
        throw std::runtime_error("improper -pixelRange arguments");
      }
      params.pixelRange.lower.x = atoi(argv[argIndex++]);
      int y0 = atoi(argv[argIndex++]);
      params.pixelRange.upper.x = atoi(argv[argIndex++]);
      int y1 = atoi(argv[argIndex++]);
      params.pixelRange.lower.y = params.windowSize.y-y1;
      params.pixelRange.upper.y = params.windowSize.y-y0;
    } else if (switchArg == "-disable-vsync") {
      params.disableVSync = true;
    } else if (switchArg == "-gridOrigin") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -gridOrigin arguments");
      }

      const std::string gridOriginX(argv[argIndex++]);
      const std::string gridOriginY(argv[argIndex++]);
      const std::string gridOriginZ(argv[argIndex++]);

      params.gridOrigin =
          vec3f(stof(gridOriginX), stof(gridOriginY), stof(gridOriginZ));
    } else if (switchArg == "-gridSpacing") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -gridSpacing arguments");
      }

      const std::string gridSpacingX(argv[argIndex++]);
      const std::string gridSpacingY(argv[argIndex++]);
      const std::string gridSpacingZ(argv[argIndex++]);

      params.gridSpacing =
          vec3f(stof(gridSpacingX), stof(gridSpacingY), stof(gridSpacingZ));
    } else if (switchArg == "-gridDimensions") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -gridDimensions arguments");
      }

      const std::string dimX(argv[argIndex++]);
      const std::string dimY(argv[argIndex++]);
      const std::string dimZ(argv[argIndex++]);

      params.dimensions = vec3i(stoi(dimX), stoi(dimY), stoi(dimZ));
    } else if (switchArg == "-valueRange") {
      if (argc < argIndex + 2) {
        throw std::runtime_error("improper -valueRange arguments");
      }

      const std::string rangeLower(argv[argIndex++]);
      const std::string rangeUpper(argv[argIndex++]);

      params.initialValueRange = range1f(stof(rangeLower), stof(rangeUpper));
    } else if (switchArg == "-voxelType") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -voxelType argument");
      }

      std::map<std::string, VKLDataType> stringToVKLDataType = {
          {"uchar", VKL_UCHAR},
          {"short", VKL_SHORT},
          {"ushort", VKL_USHORT},
          {"half", VKL_HALF},
          {"float", VKL_FLOAT},
          {"double", VKL_DOUBLE}};

      params.voxelTypeString = std::string(argv[argIndex++]);

      if (!stringToVKLDataType.count(params.voxelTypeString)) {
        throw std::runtime_error("unsupported -voxelType specified");
      }

      params.voxelType = stringToVKLDataType[params.voxelTypeString];
    } else if (switchArg == "-multiAttribute") {
      params.multiAttribute = true;
    } else if (switchArg == "-motionBlur") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -motionBlur arguments");
      }

      std::string motionBlurType = argv[argIndex++];

      if (motionBlurType == "structured") {
        params.motionBlurStructured = true;
      } else if (motionBlurType == "unstructured") {
        params.motionBlurUnstructured = true;
      } else {
        throw std::runtime_error("improper -motionBlur arguments");
      }
    } else if (switchArg == "-file") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -file arguments");
      }

      params.filename = argv[argIndex++];
    } else if (switchArg == "-field") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -field arguments");
      }

      params.field = argv[argIndex++];
    } else if (switchArg == "-filter") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -filter arguments");
      }

      params.haveFilter           = true;
      const std::string filterArg = argv[argIndex++];
      if (filterArg == "trilinear")
        params.filter = VKL_FILTER_TRILINEAR;
      else if (filterArg == "tricubic")
        params.filter = VKL_FILTER_TRICUBIC;
      else if (filterArg == "nearest")
        params.filter = VKL_FILTER_NEAREST;
      else
        throw std::runtime_error("unsupported -filter specified");
      params.gradientFilter = params.filter;
    } else if (switchArg == "-numParticles") {
      params.numParticles = std::stoul(argv[argIndex++]);
    } else if (switchArg == "-renderer") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -renderer arguments");
      }

      params.rendererType = argv[argIndex++];
    } else if (switchArg == "-innerNodeOutput") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -innerNodeOutput arguments");
      }
      params.innerNodeOutput = argv[argIndex++];
    } else if (switchArg == "-innerNodeMaxDepth") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -innerNodeMaxDepth arguments");
      }
      params.innerNodeMaxDepth = stoi(std::string(argv[argIndex++]));
    } else if (switchArg == "-help") {
      usage(argv[0]);
      return false;
    } else {
      std::cerr << "unknown argument " << switchArg << std::endl;
      usage(argv[0]);
      throw std::runtime_error("unknown switch argument");
    }
  }

  if (area(params.pixelRange) <= 0 || params.pixelRange.lower.x < 0 ||
      params.pixelRange.lower.y < 0 ||
      params.pixelRange.upper.x > params.windowSize.x ||
      params.pixelRange.upper.y > params.windowSize.y) {
    throw std::runtime_error("invalid pixel range");
  }

  if (params.field.empty()) {
    if (params.filename.empty())
      params.field = "wavelet";
    else
      params.field = "density";
  }

  if (params.haveFilter && params.gridType != "vdb" &&
      params.gridType != "structuredRegular" &&
      params.gridType != "structuredSpherical") {
    std::cerr << "warning: -filter has no effect on " << params.gridType
              << " volumes" << std::endl;
  }

  // generate params.gridOrigin and params.gridSpacing if not specified on the
  // command-line
  if (std::isnan(params.gridOrigin.x) || std::isnan(params.gridSpacing.x)) {
    const float boundingBoxSize = 2.f;

    if (params.gridType == "structuredSpherical") {
      ProceduralStructuredSphericalVolume<>::generateGridParameters(
          params.dimensions,
          boundingBoxSize,
          params.gridOrigin,
          params.gridSpacing);
    } else {
      // all other grid types can use values generated for structured regular
      // volumes
      ProceduralStructuredRegularVolume<>::generateGridParameters(
          params.dimensions,
          boundingBoxSize,
          params.gridOrigin,
          params.gridSpacing);
    }
  }

  return true;
}

void setupVolume(ViewerParams &params,
                 std::shared_ptr<TestingVolume> &testingVolume)
{
  if (!params.filename.empty()) {
    std::cout << "filename:       " << params.filename << std::endl;

    std::string ext = params.filename.substr(params.filename.size() - 4);
    std::for_each(ext.begin(), ext.end(), [](char &c) { c = ::tolower(c); });
    if (ext == ".vdb") {
      params.gridType = "vdb";
      // avoid deferred loading when exporting innerNodes to ensure exported
      // value ranges represent the full data
      auto vol = std::shared_ptr<OpenVdbVolume>(
          OpenVdbVolume::loadVdbFile(getOpenVKLDevice(),
                                     params.filename,
                                     params.field,
                                     params.filter,
                                     params.innerNodeOutput.empty()));
      testingVolume  = std::move(vol);
      params.haveVdb = true;
    } else if (ext == ".rwh") {
      testingVolume = std::shared_ptr<RawHFileStructuredVolume>(
          new RawHFileStructuredVolume(params.filename,
                                       params.gridType,
                                       params.gridOrigin,
                                       params.gridSpacing));
    } else {
      testingVolume = std::shared_ptr<RawFileStructuredVolume>(
          new RawFileStructuredVolume(params.filename,
                                      params.gridType,
                                      params.dimensions,
                                      params.gridOrigin,
                                      params.gridSpacing,
                                      params.voxelType));
    }
  } else {
    if (params.gridType == "structuredRegular") {
      TemporalConfig temporalConfig;

      if (params.motionBlurStructured) {
        temporalConfig =
            TemporalConfig(TemporalConfig::Structured,
                           params.motionBlurStructuredNumTimesteps);
      } else if (params.motionBlurUnstructured) {
        if (params.field == "sphere" && !params.multiAttribute) {
          temporalConfig = TemporalConfig(TemporalConfig::Unstructured, 256);
          temporalConfig.useTemporalCompression = true;
          temporalConfig.temporalCompressionThreshold = 0.05f;
        } else {
          temporalConfig =
              TemporalConfig(params.motionBlurUnstructuredTimeSamples);
        }
      }

      if (params.multiAttribute) {
        testingVolume = std::shared_ptr<TestingStructuredVolumeMulti>(
            generateMultiAttributeStructuredRegularVolume(
                params.dimensions,
                params.gridOrigin,
                params.gridSpacing,
                temporalConfig,
                VKL_DATA_SHARED_BUFFER,
                false));
      } else {
        if (params.voxelType == VKL_UCHAR) {
          if (params.field == "xyz") {
            testingVolume = std::make_shared<XYZStructuredRegularVolumeUChar>(
                params.dimensions,
                params.gridOrigin,
                params.gridSpacing,
                temporalConfig);
          } else if (params.field == "sphere") {
            testingVolume =
                std::make_shared<SphereStructuredRegularVolumeUChar>(
                    params.dimensions,
                    params.gridOrigin,
                    params.gridSpacing,
                    temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletStructuredRegularVolumeUChar>(
                    params.dimensions,
                    params.gridOrigin,
                    params.gridSpacing,
                    temporalConfig);
          }
        } else if (params.voxelType == VKL_SHORT) {
          if (params.field == "xyz") {
            testingVolume = std::make_shared<XYZStructuredRegularVolumeShort>(
                params.dimensions,
                params.gridOrigin,
                params.gridSpacing,
                temporalConfig);
          } else if (params.field == "sphere") {
            testingVolume =
                std::make_shared<SphereStructuredRegularVolumeShort>(
                    params.dimensions,
                    params.gridOrigin,
                    params.gridSpacing,
                    temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletStructuredRegularVolumeShort>(
                    params.dimensions,
                    params.gridOrigin,
                    params.gridSpacing,
                    temporalConfig);
          }
        } else if (params.voxelType == VKL_USHORT) {
          if (params.field == "xyz") {
            testingVolume = std::make_shared<XYZStructuredRegularVolumeUShort>(
                params.dimensions,
                params.gridOrigin,
                params.gridSpacing,
                temporalConfig);
          } else if (params.field == "sphere") {
            testingVolume =
                std::make_shared<SphereStructuredRegularVolumeUShort>(
                    params.dimensions,
                    params.gridOrigin,
                    params.gridSpacing,
                    temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletStructuredRegularVolumeUShort>(
                    params.dimensions,
                    params.gridOrigin,
                    params.gridSpacing,
                    temporalConfig);
          }
        } else if (params.voxelType == VKL_HALF) {
          if (params.field == "xyz") {
            testingVolume = std::make_shared<XYZStructuredRegularVolumeHalf>(
                params.dimensions,
                params.gridOrigin,
                params.gridSpacing,
                temporalConfig);
          } else if (params.field == "sphere") {
            testingVolume = std::make_shared<SphereStructuredRegularVolumeHalf>(
                params.dimensions,
                params.gridOrigin,
                params.gridSpacing,
                temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletStructuredRegularVolumeHalf>(
                    params.dimensions,
                    params.gridOrigin,
                    params.gridSpacing,
                    temporalConfig);
          }
        } else if (params.voxelType == VKL_FLOAT) {
          if (params.field == "xyz") {
            testingVolume = std::make_shared<XYZStructuredRegularVolumeFloat>(
                params.dimensions,
                params.gridOrigin,
                params.gridSpacing,
                temporalConfig);
          } else if (params.field == "sphere") {
            testingVolume =
                std::make_shared<SphereStructuredRegularVolumeFloat>(
                    params.dimensions,
                    params.gridOrigin,
                    params.gridSpacing,
                    temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletStructuredRegularVolumeFloat>(
                    params.dimensions,
                    params.gridOrigin,
                    params.gridSpacing,
                    temporalConfig);
          }
        } else if (params.voxelType == VKL_DOUBLE) {
          if (params.field == "xyz") {
            testingVolume = std::make_shared<XYZStructuredRegularVolumeDouble>(
                params.dimensions,
                params.gridOrigin,
                params.gridSpacing,
                temporalConfig);
          } else if (params.field == "sphere") {
            testingVolume =
                std::make_shared<SphereStructuredRegularVolumeDouble>(
                    params.dimensions,
                    params.gridOrigin,
                    params.gridSpacing,
                    temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletStructuredRegularVolumeDouble>(
                    params.dimensions,
                    params.gridOrigin,
                    params.gridSpacing,
                    temporalConfig);
          }
        } else {
          throw std::runtime_error(
              "cannot create procedural structuredRegular volume for unknown "
              "voxel type");
        }
      }
    }

    else if (params.gridType == "structuredSpherical") {
      if (params.voxelType == VKL_UCHAR) {
        if (params.field == "xyz") {
          testingVolume = std::make_shared<XYZStructuredSphericalVolumeUChar>(
              params.dimensions, params.gridOrigin, params.gridSpacing);
        } else if (params.field == "sphere") {
          testingVolume =
              std::make_shared<SphereStructuredSphericalVolumeUChar>(
                  params.dimensions, params.gridOrigin, params.gridSpacing);
        } else {
          testingVolume =
              std::make_shared<WaveletStructuredSphericalVolumeUChar>(
                  params.dimensions, params.gridOrigin, params.gridSpacing);
        }
      } else if (params.voxelType == VKL_SHORT) {
        if (params.field == "xyz") {
          testingVolume = std::make_shared<XYZStructuredSphericalVolumeShort>(
              params.dimensions, params.gridOrigin, params.gridSpacing);
        } else if (params.field == "sphere") {
          testingVolume =
              std::make_shared<SphereStructuredSphericalVolumeShort>(
                  params.dimensions, params.gridOrigin, params.gridSpacing);
        } else {
          testingVolume =
              std::make_shared<WaveletStructuredSphericalVolumeShort>(
                  params.dimensions, params.gridOrigin, params.gridSpacing);
        }
      } else if (params.voxelType == VKL_USHORT) {
        if (params.field == "xyz") {
          testingVolume = std::make_shared<XYZStructuredSphericalVolumeUShort>(
              params.dimensions, params.gridOrigin, params.gridSpacing);
        } else if (params.field == "sphere") {
          testingVolume =
              std::make_shared<SphereStructuredSphericalVolumeUShort>(
                  params.dimensions, params.gridOrigin, params.gridSpacing);
        } else {
          testingVolume =
              std::make_shared<WaveletStructuredSphericalVolumeUShort>(
                  params.dimensions, params.gridOrigin, params.gridSpacing);
        }
      } else if (params.voxelType == VKL_HALF) {
        if (params.field == "xyz") {
          testingVolume = std::make_shared<XYZStructuredSphericalVolumeHalf>(
              params.dimensions, params.gridOrigin, params.gridSpacing);
        } else if (params.field == "sphere") {
          testingVolume = std::make_shared<SphereStructuredSphericalVolumeHalf>(
              params.dimensions, params.gridOrigin, params.gridSpacing);
        } else {
          testingVolume =
              std::make_shared<WaveletStructuredSphericalVolumeHalf>(
                  params.dimensions, params.gridOrigin, params.gridSpacing);
        }
      } else if (params.voxelType == VKL_FLOAT) {
        if (params.field == "xyz") {
          testingVolume = std::make_shared<XYZStructuredSphericalVolumeFloat>(
              params.dimensions, params.gridOrigin, params.gridSpacing);
        } else if (params.field == "sphere") {
          testingVolume =
              std::make_shared<SphereStructuredSphericalVolumeFloat>(
                  params.dimensions, params.gridOrigin, params.gridSpacing);
        } else {
          testingVolume =
              std::make_shared<WaveletStructuredSphericalVolumeFloat>(
                  params.dimensions, params.gridOrigin, params.gridSpacing);
        }
      } else if (params.voxelType == VKL_DOUBLE) {
        if (params.field == "xyz") {
          testingVolume = std::make_shared<XYZStructuredSphericalVolumeDouble>(
              params.dimensions, params.gridOrigin, params.gridSpacing);
        } else if (params.field == "sphere") {
          testingVolume =
              std::make_shared<SphereStructuredSphericalVolumeDouble>(
                  params.dimensions, params.gridOrigin, params.gridSpacing);
        } else {
          testingVolume =
              std::make_shared<WaveletStructuredSphericalVolumeDouble>(
                  params.dimensions, params.gridOrigin, params.gridSpacing);
        }
      } else {
        throw std::runtime_error(
            "cannot create procedural structuredSpherical volume for unknown "
            "voxel type");
      }
    }

    else if (params.gridType == "unstructured") {
      if (params.field == "xyz") {
        testingVolume = std::make_shared<XYZUnstructuredProceduralVolume>(
            params.dimensions,
            params.gridOrigin,
            params.gridSpacing,
            VKL_HEXAHEDRON,
            false);
      } else if (params.field == "sphere") {
        testingVolume = std::make_shared<SphereUnstructuredProceduralVolume>(
            params.dimensions,
            params.gridOrigin,
            params.gridSpacing,
            VKL_HEXAHEDRON,
            false);
      } else if (params.field == "mixed") {
        testingVolume = std::make_shared<UnstructuredVolumeMixedSimple>();
      } else {
        testingVolume = std::make_shared<WaveletUnstructuredProceduralVolume>(
            params.dimensions,
            params.gridOrigin,
            params.gridSpacing,
            VKL_HEXAHEDRON,
            false);
      }
    }

    else if (params.gridType == "amr") {
      if (params.voxelType == VKL_FLOAT) {
        testingVolume = std::shared_ptr<ProceduralShellsAMRVolume<>>(
            new ProceduralShellsAMRVolume<>(
                params.dimensions, params.gridOrigin, params.gridSpacing));
      } else {
        throw std::runtime_error(
            "cannot create procedural AMR volume for non-float voxel type");
      }
    }

    else if (params.gridType == "vdb") {
      TemporalConfig temporalConfig;
      if (params.motionBlurStructured) {
        temporalConfig =
            TemporalConfig(TemporalConfig::Structured,
                           params.motionBlurStructuredNumTimesteps);
      } else if (params.motionBlurUnstructured) {
        if (params.field == "sphere" && !params.multiAttribute) {
          temporalConfig = TemporalConfig(TemporalConfig::Unstructured, 256);
          temporalConfig.useTemporalCompression = true;
          temporalConfig.temporalCompressionThreshold = 0.05f;
        } else {
          temporalConfig =
              TemporalConfig(params.motionBlurUnstructuredTimeSamples);
        }
      }

      if (params.multiAttribute) {
        if (params.voxelType == VKL_HALF) {
          testingVolume = std::shared_ptr<ProceduralVdbVolumeMulti>(
              generateMultiAttributeVdbVolumeHalf(getOpenVKLDevice(),
                                                  params.dimensions,
                                                  params.gridOrigin,
                                                  params.gridSpacing,
                                                  params.filter,
                                                  VKL_DATA_SHARED_BUFFER,
                                                  false));
        } else if (params.voxelType == VKL_FLOAT) {
          testingVolume = std::shared_ptr<ProceduralVdbVolumeMulti>(
              generateMultiAttributeVdbVolumeFloat(getOpenVKLDevice(),
                                                   params.dimensions,
                                                   params.gridOrigin,
                                                   params.gridSpacing,
                                                   params.filter,
                                                   VKL_DATA_SHARED_BUFFER,
                                                   false));
        } else {
          throw std::runtime_error(
              "can only create procedural VDB multi-attribute volumes for "
              "VKL_HALF or VKL_FLOAT voxel types");
        }
      } else {
        if (params.voxelType == VKL_HALF) {
          if (params.field == "xyz") {
            testingVolume =
                std::make_shared<XYZVdbVolumeHalf>(getOpenVKLDevice(),
                                                   params.dimensions,
                                                   params.gridOrigin,
                                                   params.gridSpacing,
                                                   params.filter,
                                                   temporalConfig);
          } else if (params.field == "sphere") {
            testingVolume =
                std::make_shared<SphereVdbVolumeHalf>(getOpenVKLDevice(),
                                                      params.dimensions,
                                                      params.gridOrigin,
                                                      params.gridSpacing,
                                                      params.filter,
                                                      temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletVdbVolumeHalf>(getOpenVKLDevice(),
                                                       params.dimensions,
                                                       params.gridOrigin,
                                                       params.gridSpacing,
                                                       params.filter,
                                                       temporalConfig);
          }
        } else if (params.voxelType == VKL_FLOAT) {
          if (params.field == "xyz") {
            testingVolume =
                std::make_shared<XYZVdbVolumeFloat>(getOpenVKLDevice(),
                                                    params.dimensions,
                                                    params.gridOrigin,
                                                    params.gridSpacing,
                                                    params.filter,
                                                    temporalConfig);
          } else if (params.field == "sphere") {
            testingVolume =
                std::make_shared<SphereVdbVolumeFloat>(getOpenVKLDevice(),
                                                       params.dimensions,
                                                       params.gridOrigin,
                                                       params.gridSpacing,
                                                       params.filter,
                                                       temporalConfig);
          } else {
            testingVolume =
                std::make_shared<WaveletVdbVolumeFloat>(getOpenVKLDevice(),
                                                        params.dimensions,
                                                        params.gridOrigin,
                                                        params.gridSpacing,
                                                        params.filter,
                                                        temporalConfig);
          }
        } else {
          throw std::runtime_error(
              "can only create procedural VDB volumes for VKL_HALF or "
              "VKL_FLOAT voxel types");
        }
      }

      params.haveVdb = true;
    }

    else if (params.gridType == "particle") {
      testingVolume =
          std::make_shared<ProceduralParticleVolume>(params.numParticles);
    }

    else {
      throw std::runtime_error("unknown gridType specified");
    }
  }

  params.maxIteratorDepth =
      (params.gridType == "vdb" ? VKL_VDB_NUM_LEVELS - 2 : 6);

  params.elementaryCellIteration = false;

  if (params.haveFilter && !params.haveVdb) {
    std::cerr << "warning: -filter has no effect on " << params.gridType
              << " volumes" << std::endl;
  }
}

void setupSampler(const ViewerParams &params, Scene &scene)
{
  vklSetInt(scene.sampler, "filter", params.filter);
  vklSetInt(scene.sampler, "gradientFilter", params.gradientFilter);
  vklSetInt(scene.sampler, "maxSamplingDepth", params.maxSamplingDepth);
  vklSetInt(scene.sampler, "maxIteratorDepth", params.maxIteratorDepth);
  vklSetBool(
      scene.sampler, "elementaryCellIteration", params.elementaryCellIteration);
  vklCommit(scene.sampler);
}

void setupScene(const ViewerParams &params,
                TestingVolume *testingVolume,
                Scene &scene)
{
  scene.updateVolume(testingVolume->getVKLVolume(getOpenVKLDevice()));
  setupSampler(params, scene);
}

void logToOutput(const ViewerParams &params, const Scene &scene)
{
  std::cout << "renderer:       " << params.rendererType << std::endl;
  std::cout << "gridType:       " << params.gridType << std::endl;
  std::cout << "gridDimensions: " << params.dimensions << std::endl;
  std::cout << "gridOrigin:     " << params.gridOrigin << std::endl;
  std::cout << "gridSpacing:    " << params.gridSpacing << std::endl;
  std::cout << "voxelType:      " << params.voxelTypeString << std::endl;
  std::cout << "field:          " << params.field << std::endl;

  vkl_box3f bbox = vklGetBoundingBox(scene.volume);

  std::cout << "boundingBox:    "
            << "(" << bbox.lower.x << ", " << bbox.lower.y << ", "
            << bbox.lower.z << ") -> (" << bbox.upper.x << ", " << bbox.upper.y
            << ", " << bbox.upper.z << ")" << std::endl;
}

void interactiveRender(ViewerParams &params,
                       Scene &scene,
                       TestingVolume *testingVolume)
{
  VKLObserver leafAccessObserver = nullptr;
  {
    auto vdbVolume = dynamic_cast<OpenVdbVolume *>(testingVolume);
    if (vdbVolume)
      leafAccessObserver = vdbVolume->newLeafAccessObserver(scene.sampler);
  }
  TransferFunction transferFunction;
  std::vector<float> isoValues;
  auto glfwVKLWindow = rkcommon::make_unique<GLFWVKLWindow>(
      params.windowSize, scene, params.rendererType, params.disableVSync);

  glfwVKLWindow->setRenderPixelRange(params.pixelRange);

  glfwVKLWindow->registerImGuiCallback([&]() {
    bool changed = false;

    static int whichRenderer = 0;
    if (ImGui::Combo("renderer",
                     &whichRenderer,
                     "density_pathtracer\0hit_iterator\0ray_march_"
                     "iterator\0interval_iterator_debug\0\0")) {
      switch (whichRenderer) {
      case 0:
        params.rendererType = "density_pathtracer";
        break;
      case 1:
        params.rendererType = "hit_iterator";
        break;
      case 2:
        params.rendererType = "ray_march_iterator";
        break;
      case 3:
        params.rendererType = "interval_iterator_debug";
        break;
      default:
        break;
      }

      glfwVKLWindow->setActiveRenderer(params.rendererType);
    }

    bool samplerParamsChanged = false;

    // maxIteratorDepth parameter currently only applies to unstructured,
    // particle, and AMR volume samplers (special case below for vdb).
    if (params.gridType == "unstructured" || params.gridType == "particle" ||
        params.gridType == "amr") {
      if (ImGui::SliderInt(
              "maxIteratorDepth", &params.maxIteratorDepth, 0, 31)) {
        samplerParamsChanged = true;
      }
    }

    // elementaryCellIteration parameter currently only appies to unstructured
    // volume samplers.
    if (params.gridType == "unstructured") {
      if (ImGui::Checkbox("elementaryCellIteration",
                          &params.elementaryCellIteration)) {
        samplerParamsChanged = true;
      }
    }

    if (params.gridType == "structuredRegular" ||
        params.gridType == "structuredSpherical" || params.gridType == "vdb") {
      static std::map<VKLFilter, const char *> filters = {
          {VKL_FILTER_TRICUBIC, "tricubic"},
          {VKL_FILTER_NEAREST, "nearest"},
          {VKL_FILTER_TRILINEAR, "trilinear"}};

      if (ImGui::BeginCombo("filter", filters[params.filter])) {
        for (auto it : filters) {
          if (it.first == VKL_FILTER_TRICUBIC && params.gridType != "vdb")
            continue;
          const bool isSelected = (params.filter == it.first);
          if (ImGui::Selectable(filters[it.first], isSelected)) {
            params.filter        = it.first;
            samplerParamsChanged = true;
          }
          if (isSelected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      if (ImGui::BeginCombo("gradientFilter", filters[params.gradientFilter])) {
        for (auto it : filters) {
          if (it.first == VKL_FILTER_TRICUBIC && params.gridType != "vdb")
            continue;
          const bool isSelected = (params.filter == it.first);
          if (ImGui::Selectable(filters[it.first], isSelected)) {
            params.gradientFilter = it.first;
            samplerParamsChanged  = true;
          }
          if (isSelected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
    }

    // VDB specific parameters.
    if (params.gridType == "vdb") {
      if (ImGui::SliderInt("maxSamplingDepth",
                           &params.maxSamplingDepth,
                           0,
                           VKL_VDB_NUM_LEVELS - 1) ||
          ImGui::SliderInt("maxIteratorDepth",
                           &params.maxIteratorDepth,
                           0,
                           VKL_VDB_NUM_LEVELS - 1)) {
        samplerParamsChanged = true;
      }
    }

    if (samplerParamsChanged) {
      setupSampler(params, scene);
      changed = true;
    }

    static int useISPC = params.useISPC;
    if (ImGui::Combo("OpenVKL API used", &useISPC, "C scalar\0ISPC\0\0")) {
      glfwVKLWindow->setUseISPC(useISPC);
      changed = true;
    }

    static int spp = 1;
    if (ImGui::SliderInt("spp", &spp, 1, 16)) {
      auto &renderer = glfwVKLWindow->currentRenderer();
      renderer.setParam<int>("spp", spp);
      renderer.commit();
    }

    if (params.rendererType == "ray_march_iterator") {
      changed |= addSamplingRateUI(*glfwVKLWindow);
    }

    if (params.rendererType == "density_pathtracer") {
      changed |= addPathTracerUI(*glfwVKLWindow, scene);
    }

    if (params.rendererType == "hit_iterator") {
      if (addIsosurfacesUI(*glfwVKLWindow, isoValues)) {
        changed = true;
        scene.updateValueSelector(transferFunction, isoValues);
      }
    }

    if (params.rendererType == "interval_iterator_debug") {
      changed |= addIntervalIteratorDebugUI(*glfwVKLWindow);
    }

    auto transferFunctionUpdatedCallback =
        [&](const range1f &valueRange,
            const std::vector<vec4f> &colorsAndOpacities) {
          transferFunction = TransferFunction{valueRange, colorsAndOpacities};
          scene.tfColorsAndOpacities =
              transferFunction.colorsAndOpacities.data();
          scene.tfNumColorsAndOpacities =
              transferFunction.colorsAndOpacities.size();
          scene.tfValueRange = valueRange;
          scene.updateValueSelector(transferFunction, isoValues);
          glfwVKLWindow->resetAccumulation();
        };

    static TransferFunctionWidget transferFunctionWidget(
        transferFunctionUpdatedCallback, params.initialValueRange);
    transferFunctionWidget.updateUI();

    if (changed) {
      glfwVKLWindow->resetAccumulation();
    }
  });

  glfwVKLWindow->registerEndOfFrameCallback([&]() {
    auto vdbVolume = dynamic_cast<OpenVdbVolume *>(testingVolume);
    if (vdbVolume && vdbVolume->updateVolume(leafAccessObserver)) {
      scene.updateVolume(vdbVolume->getVKLVolume(getOpenVKLDevice()));
      setupSampler(params, scene);
      scene.updateValueSelector(transferFunction, isoValues);

      if (leafAccessObserver)
        vklRelease(leafAccessObserver);
      leafAccessObserver = vdbVolume->newLeafAccessObserver(scene.sampler);

      glfwVKLWindow->resetAccumulation();
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwVKLWindow->mainLoop();

  if (leafAccessObserver) {
    vklRelease(leafAccessObserver);
  }

  glfwVKLWindow.reset();
}

void imageWrite(ViewerParams &params,
                Scene &scene,
                TestingVolume *testingVolume)
{
  static TransferFunction transferFunction;

  scene.tfValueRange            = box1f(0.f, 1.f);
  scene.tfColorsAndOpacities    = transferFunction.colorsAndOpacities.data();
  scene.tfNumColorsAndOpacities = transferFunction.colorsAndOpacities.size();

  // and a default value selector, with default isovalues
  scene.updateValueSelector(transferFunction,
                            std::vector<float>{-1.f, 0.f, 1.f});
  auto window = rkcommon::make_unique<VKLWindow>(
      params.windowSize, scene, params.rendererType);

  window->setUseISPC(params.useISPC);
  window->setRenderPixelRange(params.pixelRange);
  window->render();

  // save image on completion of benchmark; note we apparently have no way to
  // get the formal benchmark name, so we'll create one here
  std::stringstream ss;
  ss << params.fileNameOut;
  window->savePPM(ss.str());
}

int main(int argc, const char **argv)
{
  ViewerParams params;
  if (!parseCommandLine(argc, argv, params)) {
    return 1;
  }

  initializeOpenVKL();

  std::shared_ptr<TestingVolume> testingVolume;
  setupVolume(params, testingVolume);

  Scene scene;
  setupScene(params, testingVolume.get(), scene);

  logToOutput(params, scene);

  if (params.gridType == "vdb" && !params.innerNodeOutput.empty()) {
    openvkl::utility::vdb::exportInnerNodes(
        params.innerNodeOutput,
        params.innerNodeMaxDepth,
        testingVolume->getVKLVolume(getOpenVKLDevice()));
  }

  if (params.interactive) {
    interactiveRender(params, scene, testingVolume.get());
  } else {
    imageWrite(params, scene, testingVolume.get());
  }

  // cleanly shut VKL down
  scene = Scene();
  testingVolume.reset();
  shutdownOpenVKL();

  return 0;
}
