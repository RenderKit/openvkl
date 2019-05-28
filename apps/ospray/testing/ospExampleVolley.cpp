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
#include <memory>
#include <random>
#include "GLFWOSPRayWindow.h"
#include "OSPRayVolleyTestScene.h"
#include "TransferFunctionWidget.h"

using namespace ospcommon;
using namespace volley::testing;

bool addSamplingRateUI(std::shared_ptr<OSPRayVolleyTestScene> testScene)
{
  static float samplingRate = 1.f;
  if (ImGui::SliderFloat("samplingRate", &samplingRate, 0.01f, 4.f)) {
    ospSet1f(testScene->getRenderer(), "samplingRate", samplingRate);
    ospCommit(testScene->getRenderer());
    return true;
  }
  return false;
}

bool addPathTracerUI(std::shared_ptr<OSPRayVolleyTestScene> testScene)
{
  bool changed = false;

  static float sigmaTScale = 1.f;
  if (ImGui::SliderFloat("sigmaTScale", &sigmaTScale, 0.001f, 100.f)) {
    ospSet1f(testScene->getRenderer(), "sigmaTScale", sigmaTScale);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static float sigmaSScale = 1.f;
  if (ImGui::SliderFloat("sigmaSScale", &sigmaSScale, 0.01f, 1.f)) {
    ospSet1f(testScene->getRenderer(), "sigmaSScale", sigmaSScale);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static int maxNumScatters = 1;
  if (ImGui::SliderInt("maxNumScatters", &maxNumScatters, 1, 32)) {
    ospSet1i(testScene->getRenderer(), "maxNumScatters", maxNumScatters);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static bool useRatioTracking = false;
  if (ImGui::Checkbox("useRatioTracking", &useRatioTracking)) {
    ospSet1i(testScene->getRenderer(), "useRatioTracking", useRatioTracking);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static float ambientLightIntensity = 1.f;
  if (ImGui::SliderFloat(
          "ambientLightIntensity", &ambientLightIntensity, 0.f, 10.f)) {
    ospSet1f(testScene->getRenderer(),
             "ambientLightIntensity",
             ambientLightIntensity);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static float directionalLightIntensity = 1.f;
  if (ImGui::SliderFloat(
          "directionalLightIntensity", &directionalLightIntensity, 0.f, 10.f)) {
    ospSet1f(testScene->getRenderer(),
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
    ospSet1f(testScene->getRenderer(),
             "directionalLightAngularDiameter",
             directionalLightAngularDiameter);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  static float directionalLightAzimuth = 0.f;
  if (ImGui::SliderFloat(
          "directionalLightAzimuth", &directionalLightAzimuth, -180.f, 180.f)) {
    ospSet1f(testScene->getRenderer(),
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
    ospSet1f(testScene->getRenderer(),
             "directionalLightElevation",
             directionalLightElevation);
    ospCommit(testScene->getRenderer());
    changed = true;
  }

  return changed;
}

bool addIsosurfacesUI(std::shared_ptr<OSPRayVolleyTestScene> testScene)
{
  static bool showIsosurfaces = false;

  static constexpr int maxNumIsosurfaces = 3;

  struct IsosurfaceParameters
  {
    bool enabled{false};
    float isovalue{0.f};
  };

  static std::array<IsosurfaceParameters, maxNumIsosurfaces> isosurfaces;

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

int main(int argc, const char **argv)
{
  if (argc < 2) {
    std::cerr
        << "usage: " << argv[0]
        << " <simple_native | simple_volley | volley_ray_iterator_surface "
           "| volley_ray_iterator_volume | volley_ray_iterator | "
           "volley_pathtracer> [-file <float.raw> <dimX> <dimY> <dimZ>]"
        << std::endl;
    return 1;
  }

  initializeOSPRay();
  initializeVolley();

  std::string rendererType(argv[1]);
  std::cout << "using renderer: " << rendererType << std::endl;

  std::shared_ptr<TestingStructuredVolume> testingStructuredVolume;

  if (argc == 2) {
    const vec3i dimensions(256);
    const vec3f gridOrigin(-1.f);
    const vec3f gridSpacing(2.f / float(dimensions.x));

    testingStructuredVolume = std::shared_ptr<WaveletProceduralVolume>(
        new WaveletProceduralVolume(dimensions, gridOrigin, gridSpacing));
  } else if (argc > 2) {
    std::string switchArg(argv[2]);

    if (switchArg == "-file") {
      if (argc != 7) {
        throw std::runtime_error("improper -file arguments");
      }

      const std::string filename(argv[3]);
      const std::string dimX(argv[4]);
      const std::string dimY(argv[5]);
      const std::string dimZ(argv[6]);

      const vec3i dimensions(stoi(dimX), stoi(dimY), stoi(dimZ));

      // fit it into a unit cube
      const float normalizedGridSpacing = reduce_min(1.f / dimensions);

      const vec3f gridOrigin(-0.5f * dimensions * normalizedGridSpacing);
      const vec3f gridSpacing(normalizedGridSpacing);

      testingStructuredVolume =
          std::shared_ptr<RawFileStructuredVolume>(new RawFileStructuredVolume(
              filename, dimensions, gridOrigin, gridSpacing));

    } else {
      throw std::runtime_error("unknown switch argument");
    }
  }

  std::shared_ptr<OSPRayVolleyTestScene> testScene(
      new OSPRayVolleyTestScene(rendererType, testingStructuredVolume));

  auto glfwOSPRayWindow = std::shared_ptr<GLFWOSPRayWindow>(
      new GLFWOSPRayWindow(vec2i{1024, 1024},
                           testScene->getBoundingBox(),
                           testScene->getWorld(),
                           testScene->getRenderer()));

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    bool changed = false;

    if (rendererType == "volley_ray_iterator" ||
        rendererType == "volley_ray_iterator_volume") {
      changed = changed || addSamplingRateUI(testScene);
    }

    if (rendererType == "volley_pathtracer") {
      changed = changed || addPathTracerUI(testScene);
    }

    if (rendererType == "volley_ray_iterator" ||
        rendererType == "volley_ray_iterator_surface") {
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

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
