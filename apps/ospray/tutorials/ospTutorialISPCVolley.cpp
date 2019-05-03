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

int main(int argc, const char **argv)
{
  if (argc < 2) {
    std::cerr
        << "usage: " << argv[0]
        << " <simple_native> | <simple_volley> | <volley_ray_iterator_surface> "
           "| <volley_ray_iterator_volume> | <volley_ray_iterator>"
        << std::endl;
    return 1;
  }

  initializeOSPRay();
  initializeVolley();

  const vec3i dimensions(256);
  const vec3f gridOrigin(-1.f);
  const vec3f gridSpacing(2.f / float(dimensions.x));

  std::shared_ptr<WaveletProceduralVolume> proceduralVolume(
      new WaveletProceduralVolume(dimensions, gridOrigin, gridSpacing));

  std::string rendererType(argv[1]);
  std::cout << "using renderer: " << rendererType << std::endl;

  std::unique_ptr<OSPRayVolleyTestScene> testScene(
      new OSPRayVolleyTestScene(rendererType, proceduralVolume));

  auto glfwOSPRayWindow = std::unique_ptr<GLFWOSPRayWindow>(
      new GLFWOSPRayWindow(vec2i{1024, 1024},
                           testScene->getBoundingBox(),
                           testScene->getWorld(),
                           testScene->getRenderer()));

  auto transferFunctionUpdatedCallback = [&]() {
    glfwOSPRayWindow->resetAccumulation();
  };

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    static float samplingRate = 1.f;
    if (ImGui::SliderFloat("samplingRate", &samplingRate, 0.01f, 4.f)) {
      ospSet1f(testScene->getRenderer(), "samplingRate", samplingRate);
      ospCommit(testScene->getRenderer());
      glfwOSPRayWindow->resetAccumulation();
    }

    // only show isosurface UI if an appropriate renderer is selected
    if (rendererType == "volley_ray_iterator" ||
        rendererType == "volley_ray_iterator_surface") {
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

          if (ImGui::Checkbox(enabledLabel.str().c_str(),
                              &isosurface.enabled)) {
            isosurfacesChanged = true;
          }

          ImGui::SameLine();

          if (ImGui::SliderFloat(isovalueLabel.str().c_str(),
                                 &isosurface.isovalue,
                                 -1.f,
                                 1.f)) {
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

        glfwOSPRayWindow->resetAccumulation();
      }
    }

    static TransferFunctionWidget transferFunctionWidget(
        testScene->getTransferFunction(), transferFunctionUpdatedCallback);
    transferFunctionWidget.updateUI();
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
