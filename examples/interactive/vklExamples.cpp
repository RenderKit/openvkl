// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#include "AppInit.h"
#include "window/GLFWVKLWindow.h"
#include "window/TransferFunctionWidget.h"
// openvkl_testing
#include "openvkl_testing.h"
// imgui
#include <imgui.h>
// std
#include <array>
#include <map>
#include <memory>
#include <random>

using namespace ospcommon;
using namespace openvkl::testing;
using namespace openvkl::examples;

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

bool addPathTracerUI(GLFWVKLWindow &window)
{
  auto &renderer = window.currentRenderer();

  bool changed = false;

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

bool addIsosurfacesUI(GLFWVKLWindow &window)
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

  if (!initialized) {
    isosurfaces[0].isovalue = -1.f;
    isosurfaces[1].isovalue = 0.f;
    isosurfaces[2].isovalue = 1.f;

    initialized = true;
  }

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
    static std::vector<float> enabledIsovalues;
    enabledIsovalues.clear();

    if (showIsosurfaces) {
      for (const auto &isosurface : isosurfaces) {
        if (isosurface.enabled) {
          enabledIsovalues.push_back(isosurface.isovalue);
        }
      }
    }

    window.setIsovalues(enabledIsovalues);
  }

  return isosurfacesChanged;
}

void usage(const char *progname)
{
  std::cerr << "usage: " << progname << "\n"
            << "\t-renderer density_pathtracer | hit_iterator |"
               " ray_march_iterator\n"
               "\t-gridType structured_regular | structured_spherical | "
               "unstructured | amr\n"
               "\t-gridOrigin <x> <y> <z>\n"
               "\t-gridSpacing <x> <y> <z>\n"
               "\t-gridDimensions <dimX> <dimY> <dimZ>\n"
               "\t-voxelType uchar | short | ushort | float | double\n"
               "\t-file <float.raw>"
            << std::endl;
}

int main(int argc, const char **argv)
{
  std::string rendererType("density_pathtracer");
  std::string gridType("structured_regular");
  vec3i dimensions(128);
  vec3f gridOrigin(ospcommon::nan);
  vec3f gridSpacing(ospcommon::nan);
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
    } else if (switchArg == "-gridOrigin") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -gridOrigin arguments");
      }

      const std::string gridOriginX(argv[argIndex++]);
      const std::string gridOriginY(argv[argIndex++]);
      const std::string gridOriginZ(argv[argIndex++]);

      gridOrigin =
          vec3f(stof(gridOriginX), stof(gridOriginY), stof(gridOriginZ));
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
        throw std::runtime_error("improper -voxelType argument");
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

  // generate gridOrigin and gridSpacing if not specified on the command-line
  if (std::isnan(gridOrigin.x) || std::isnan(gridSpacing.x)) {
    const float boundingBoxSize = 2.f;

    if (gridType == "structured_spherical") {
      ProceduralStructuredSphericalVolume<>::generateGridParameters(
          dimensions, boundingBoxSize, gridOrigin, gridSpacing);
    } else {
      // all other grid types can use values generated for structured regular
      // volumes
      ProceduralStructuredRegularVolume<>::generateGridParameters(
          dimensions, boundingBoxSize, gridOrigin, gridSpacing);
    }
  }

  initializeOpenVKL();

  std::shared_ptr<TestingVolume> testingVolume;

  if (!filename.empty()) {
    std::cout << "filename:       " << filename << std::endl;
    testingVolume = std::shared_ptr<RawFileStructuredVolume>(
        new RawFileStructuredVolume(filename,
                                    gridType,
                                    dimensions,
                                    gridOrigin,
                                    gridSpacing,
                                    voxelType));
  } else {
    if (gridType == "structured_regular") {
      if (voxelType == VKL_UCHAR) {
        testingVolume = std::make_shared<WaveletStructuredRegularVolumeUChar>(
            dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_SHORT) {
        testingVolume = std::make_shared<WaveletStructuredRegularVolumeShort>(
            dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_USHORT) {
        testingVolume = std::make_shared<WaveletStructuredRegularVolumeUShort>(
            dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_FLOAT) {
        testingVolume = std::make_shared<WaveletStructuredRegularVolumeFloat>(
            dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_DOUBLE) {
        testingVolume = std::make_shared<WaveletStructuredRegularVolumeDouble>(
            dimensions, gridOrigin, gridSpacing);
      } else {
        throw std::runtime_error(
            "cannot create procedural volume for unknown voxel type");
      }
    }

    else if (gridType == "structured_spherical") {
      if (voxelType == VKL_UCHAR) {
        testingVolume = std::make_shared<WaveletStructuredSphericalVolumeUChar>(
            dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_SHORT) {
        testingVolume = std::make_shared<WaveletStructuredSphericalVolumeShort>(
            dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_USHORT) {
        testingVolume =
            std::make_shared<WaveletStructuredSphericalVolumeUShort>(
                dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_FLOAT) {
        testingVolume = std::make_shared<WaveletStructuredSphericalVolumeFloat>(
            dimensions, gridOrigin, gridSpacing);
      } else if (voxelType == VKL_DOUBLE) {
        testingVolume =
            std::make_shared<WaveletStructuredSphericalVolumeDouble>(
                dimensions, gridOrigin, gridSpacing);
      } else {
        throw std::runtime_error(
            "cannot create procedural volume for unknown voxel type");
      }
    }

    else if (gridType == "unstructured") {
      testingVolume = std::shared_ptr<WaveletUnstructuredProceduralVolume>(
          new WaveletUnstructuredProceduralVolume(
              dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false));
    }

    else if (gridType == "amr") {
      testingVolume = std::shared_ptr<ProceduralShellsAMRVolume<>>(
          new ProceduralShellsAMRVolume<>(dimensions, gridOrigin, gridSpacing));
    }
  }

  VKLVolume volume = testingVolume->getVKLVolume();

  std::cout << "renderer:       " << rendererType << std::endl;
  std::cout << "gridType:       " << gridType << std::endl;
  std::cout << "gridDimensions: " << dimensions << std::endl;
  std::cout << "gridOrigin:     " << gridOrigin << std::endl;
  std::cout << "gridSpacing:    " << gridSpacing << std::endl;
  std::cout << "voxelType:      " << voxelTypeString << std::endl;

  vkl_box3f bbox = vklGetBoundingBox(volume);

  std::cout << "boundingBox:    "
            << "(" << bbox.lower.x << ", " << bbox.lower.y << ", "
            << bbox.lower.z << ") -> (" << bbox.upper.x << ", " << bbox.upper.y
            << ", " << bbox.upper.z << ")" << std::endl;

  auto glfwVKLWindow = ospcommon::make_unique<GLFWVKLWindow>(
      vec2i{1024, 1024}, volume, rendererType);

  auto &renderer = glfwVKLWindow->currentRenderer();

  glfwVKLWindow->registerImGuiCallback([&]() {
    bool changed = false;

    static int whichRenderer = 0;
    if (ImGui::Combo(
            "renderer",
            &whichRenderer,
            "density_pathtracer\0hit_iterator\0ray_march_iterator\0\0")) {
      switch (whichRenderer) {
      case 0:
        rendererType = "density_pathtracer";
        break;
      case 1:
        rendererType = "hit_iterator";
        break;
      case 2:
        rendererType = "ray_march_iterator";
        break;
      default:
        break;
      }

      glfwVKLWindow->setActiveRenderer(rendererType);
    }

    static int useISPC = 1;
    if (ImGui::Combo("OpenVKL API used", &useISPC, "C scalar\0ISPC\0\0")) {
      glfwVKLWindow->setUseISPC(useISPC);
      changed = true;
    }

    static int spp = 1;
    if (ImGui::SliderInt("spp", &spp, 1, 16)) {
      renderer.setParam<int>("spp", spp);
      renderer.commit();
    }

    if (rendererType == "ray_march_iterator") {
      changed |= addSamplingRateUI(*glfwVKLWindow);
    }

    if (rendererType == "density_pathtracer") {
      changed |= addPathTracerUI(*glfwVKLWindow);
    }

    if (rendererType == "hit_iterator") {
      changed |= addIsosurfacesUI(*glfwVKLWindow);
    }

    auto transferFunctionUpdatedCallback =
        [&](const range1f &valueRange,
            const std::vector<vec4f> &colorsAndOpacities) {
          TransferFunction tf{valueRange, colorsAndOpacities};
          glfwVKLWindow->setTransferFunction(tf);
          glfwVKLWindow->resetAccumulation();
        };

    static TransferFunctionWidget transferFunctionWidget(
        transferFunctionUpdatedCallback);
    transferFunctionWidget.updateUI();

    if (changed) {
      glfwVKLWindow->resetAccumulation();
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwVKLWindow->mainLoop();

  // cleanly shut VKL down
  testingVolume.reset();
  glfwVKLWindow.reset();
  vklShutdown();

  return 0;
}
