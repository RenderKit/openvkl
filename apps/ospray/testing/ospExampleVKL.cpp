// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include <imgui.h>
#include <array>
#include <map>
#include <memory>
#include <random>
#include "GLFWOSPRayWindow.h"
#include "OSPRayVKLTestScene.h"
#include "TransferFunctionWidget.h"

using namespace ospcommon;
using namespace openvkl::testing;

bool addSamplingRateUI(std::shared_ptr<OSPRayVKLTestScene> testScene)
{
  static float samplingRate = 1.f;
  if (ImGui::SliderFloat("samplingRate", &samplingRate, 0.01f, 4.f)) {
    ospSetFloat(testScene->getRenderer(), "samplingRate", samplingRate);
    ospCommit(testScene->getRenderer());
    return true;
  }
  return false;
}

bool addPathTracerUI(std::shared_ptr<OSPRayVKLTestScene> testScene)
{
  bool changed = false;

  static float sigmaTScale = 1.f;
  if (ImGui::SliderFloat("sigmaTScale", &sigmaTScale, 0.001f, 100.f)) {
    ospSetFloat(testScene->getRenderer(), "sigmaTScale", sigmaTScale);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static float sigmaSScale = 1.f;
  if (ImGui::SliderFloat("sigmaSScale", &sigmaSScale, 0.01f, 1.f)) {
    ospSetFloat(testScene->getRenderer(), "sigmaSScale", sigmaSScale);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static int maxNumScatters = 1;
  if (ImGui::SliderInt("maxNumScatters", &maxNumScatters, 1, 32)) {
    ospSetInt(testScene->getRenderer(), "maxNumScatters", maxNumScatters);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static bool useRatioTracking = false;
  if (ImGui::Checkbox("useRatioTracking", &useRatioTracking)) {
    ospSetInt(testScene->getRenderer(), "useRatioTracking", useRatioTracking);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static float ambientLightIntensity = 1.f;
  if (ImGui::SliderFloat(
          "ambientLightIntensity", &ambientLightIntensity, 0.f, 10.f)) {
    ospSetFloat(testScene->getRenderer(),
                "ambientLightIntensity",
                ambientLightIntensity);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static float directionalLightIntensity = 1.f;
  if (ImGui::SliderFloat(
          "directionalLightIntensity", &directionalLightIntensity, 0.f, 10.f)) {
    ospSetFloat(testScene->getRenderer(),
                "directionalLightIntensity",
                directionalLightIntensity);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static float directionalLightAngularDiameter = 45.f;
  if (ImGui::SliderFloat("directionalLightAngularDiameter",
                         &directionalLightAngularDiameter,
                         0.f,
                         180.f)) {
    ospSetFloat(testScene->getRenderer(),
                "directionalLightAngularDiameter",
                directionalLightAngularDiameter);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static float directionalLightAzimuth = 0.f;
  if (ImGui::SliderFloat(
          "directionalLightAzimuth", &directionalLightAzimuth, -180.f, 180.f)) {
    ospSetFloat(testScene->getRenderer(),
                "directionalLightAzimuth",
                directionalLightAzimuth);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static float directionalLightElevation = 90.f;
  if (ImGui::SliderFloat("directionalLightElevation",
                         &directionalLightElevation,
                         -90.f,
                         90.f)) {
    ospSetFloat(testScene->getRenderer(),
                "directionalLightElevation",
                directionalLightElevation);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  return changed;
}

bool addIsosurfacesUI(std::shared_ptr<OSPRayVKLTestScene> testScene)
{
  static bool showIsosurfaces = false;

  static constexpr int maxNumIsosurfaces = 3;

  struct IsosurfaceParameters
  {
    bool enabled{true};
    float isovalue{0.f};
  };

  static std::array<IsosurfaceParameters, maxNumIsosurfaces> isosurfaces;

  isosurfaces[0].isovalue = -1.f;
  isosurfaces[1].isovalue = 0.f;
  isosurfaces[2].isovalue = 1.f;

  bool isosurfacesChanged = false;

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
    std::vector<float> enabledIsovalues;

    if (showIsosurfaces) {
      for (const auto &isosurface : isosurfaces) {
        if (isosurface.enabled) {
          enabledIsovalues.push_back(isosurface.isovalue);
        }
      }
    }

    testScene->setIsovalues(enabledIsovalues);
  }

  return isosurfacesChanged;
}

void usage(const char *progname)
{
  std::cerr << "usage: " << progname << "\n"
            << "\t-renderer simple_native | simple_vkl | "
               "vkl_ray_iterator_surface | "
               "vkl_ray_iterator_volume | vkl_ray_iterator | vkl_pathtracer\n"
               "\t-gridType structured_regular\n"
               "\t-gridSpacing <x> <y> <z>\n"
               "\t-gridDimensions <dimX> <dimY> <dimZ>\n"
               "\t-voxelType uchar | short | ushort | float | double\n"
               "\t-file <float.raw>"
            << std::endl;
}

int main(int argc, const char **argv)
{
  std::string rendererType("vkl_pathtracer");
  std::string gridType("structured_regular");
  vec3i dimensions(100);
  vec3f gridOrigin(-1.f);
  vec3f gridSpacing(-1.f);
  std::string voxelTypeString("float");
  VKLDataType voxelType(VKL_FLOAT);
  std::string filename;

  int argIndex = 1;
  while (argIndex < argc) {
    std::string switchArg(argv[argIndex++]);

    if (switchArg == "-gridType") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -gridType arguments");
      }

      gridType = std::string(argv[argIndex++]);
    } else if (switchArg == "-gridSpacing") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -gridSpacing arguments");
      }

      const std::string gridSpacingX(argv[argIndex++]);
      const std::string gridSpacingY(argv[argIndex++]);
      const std::string gridSpacingZ(argv[argIndex++]);

      gridSpacing =
          vec3f(stof(gridSpacingX), stof(gridSpacingY), stof(gridSpacingZ));
    } else if (switchArg == "-gridDimensions") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -gridDimensions arguments");
      }

      const std::string dimX(argv[argIndex++]);
      const std::string dimY(argv[argIndex++]);
      const std::string dimZ(argv[argIndex++]);

      dimensions = vec3i(stoi(dimX), stoi(dimY), stoi(dimZ));
    } else if (switchArg == "-voxelType") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -file arguments");
      }

      std::map<std::string, VKLDataType> stringToVKLDataType = {
          {"uchar", VKL_UCHAR},
          {"short", VKL_SHORT},
          {"ushort", VKL_USHORT},
          {"float", VKL_FLOAT},
          {"double", VKL_DOUBLE}};

      voxelTypeString = std::string(argv[argIndex++]);

      if (!stringToVKLDataType.count(voxelTypeString)) {
        throw std::runtime_error("unsupported -voxelType specified");
      }

      voxelType = stringToVKLDataType[voxelTypeString];
    } else if (switchArg == "-file") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -file arguments");
      }

      filename = argv[argIndex++];
    } else if (switchArg == "-renderer") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -renderer arguments");
      }

      rendererType = argv[argIndex++];
    } else if (switchArg == "-help") {
      usage(argv[0]);
      return 0;
    } else {
      std::cerr << "unknown argument " << switchArg << std::endl;
      usage(argv[0]);
      throw std::runtime_error("unknown switch argument");
    }
  }

  if (gridSpacing == vec3f(-1.f)) {
    const float normalizedGridSpacing = reduce_min(1.f / dimensions);

    gridOrigin  = vec3f(-1.f * dimensions * normalizedGridSpacing);
    gridSpacing = vec3f(2.f * normalizedGridSpacing);
  }

  std::cout << "renderer:       " << rendererType << std::endl;
  std::cout << "gridType:       " << gridType << std::endl;
  std::cout << "gridDimensions: " << dimensions << std::endl;
  std::cout << "gridOrigin:     " << gridOrigin << std::endl;
  std::cout << "gridSpacing:    " << gridSpacing << std::endl;
  std::cout << "voxelType:      " << voxelTypeString << std::endl;

  initializeOSPRay();
  initializeOpenVKL();

  std::shared_ptr<TestingStructuredVolume> testingStructuredVolume;

  if (!filename.empty()) {
    std::cout << "filename:       " << filename << std::endl;
    testingStructuredVolume = std::shared_ptr<RawFileStructuredVolume>(
        new RawFileStructuredVolume(filename,
                                    gridType,
                                    dimensions,
                                    gridOrigin,
                                    gridSpacing,
                                    voxelType));
  } else {
    if (gridType == "structured_regular") {
      if (voxelType == VKL_UCHAR) {
        testingStructuredVolume =
            std::make_shared<WaveletProceduralVolumeUchar>(
                dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_SHORT) {
        testingStructuredVolume =
            std::make_shared<WaveletProceduralVolumeShort>(
                dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_USHORT) {
        testingStructuredVolume =
            std::make_shared<WaveletProceduralVolumeUshort>(
                dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_FLOAT) {
        testingStructuredVolume =
            std::make_shared<WaveletProceduralVolumeFloat>(
                dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_DOUBLE) {
        testingStructuredVolume =
            std::make_shared<WaveletProceduralVolumeDouble>(
                dimensions, gridOrigin, gridSpacing);
      } else {
        throw std::runtime_error(
            "cannot create procedural volume for unknown voxel type");
      }
    }
  }

  std::shared_ptr<OSPRayVKLTestScene> testScene(
      new OSPRayVKLTestScene(rendererType, testingStructuredVolume));

  auto glfwOSPRayWindow = std::shared_ptr<GLFWOSPRayWindow>(
      new GLFWOSPRayWindow(vec2i{1024, 1024},
                           testScene->getBoundingBox(),
                           testScene->getWorld(),
                           testScene->getRenderer()));

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    bool changed = false;

    static int spp = 1;
    if (ImGui::SliderInt("spp", &spp, 1, 16)) {
      ospSetInt(testScene->getRenderer(), "spp", spp);
      ospCommit(testScene->getRenderer());
    }

    if (rendererType == "vkl_ray_iterator" ||
        rendererType == "vkl_ray_iterator_volume") {
      changed = changed || addSamplingRateUI(testScene);
    }

    if (rendererType == "vkl_pathtracer") {
      changed = changed || addPathTracerUI(testScene);
    }

    if (rendererType == "vkl_ray_iterator" ||
        rendererType == "vkl_ray_iterator_surface") {
      changed = changed || addIsosurfacesUI(testScene);
    }

    auto transferFunctionUpdatedCallback = [&]() {
      glfwOSPRayWindow->resetAccumulation();
    };

    static TransferFunctionWidget transferFunctionWidget(
        testScene->getTransferFunction(), transferFunctionUpdatedCallback);
    transferFunctionWidget.updateUI();

    if (changed) {
      glfwOSPRayWindow->resetAccumulation();
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut VKL and OSPRay down
  testScene.reset();
  glfwOSPRayWindow.reset();
  vklShutdown();
  ospShutdown();

  return 0;
}
