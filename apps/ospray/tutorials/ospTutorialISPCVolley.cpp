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
#include <memory>
#include <random>
#include "GLFWOSPRayWindow.h"

#include <ospray/ospray_testing/ospray_testing.h>
#include <volley/volley.h>

using namespace ospcommon;

VLYVolume createVolleyVolume()
{
  std::cout << "initializing Volley" << std::endl;

  vlyLoadModule("scalar_driver");

  VLYDriver driver = vlyNewDriver("scalar_driver");
  vlyCommitDriver(driver);
  vlySetCurrentDriver(driver);

  VLYVolume vlyVolume = vlyNewVolume("wavelet_procedural_volume");

  vec3i dimensions(512);
  vec3f gridOrigin(-1.f);
  vec3f gridSpacing(2.f / float(dimensions.x));

  vlySet3i(vlyVolume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
  vlySet3f(vlyVolume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
  vlySet3f(
      vlyVolume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

  vlyCommit(vlyVolume);

  vlyCommit(vlyVolume);

  return vlyVolume;
}

int main(int argc, const char **argv)
{
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <volley> | ..." << std::endl;
    return 1;
  }

  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    return initError;

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create the "world" model which will contain all of our geometries / volumes
  OSPModel world = ospNewModel();

  // add in generated volume and transfer function

  osp::vec2f voxelRange{-1.f, 1.f};
  OSPTransferFunction tfn = ospTestingNewTransferFunction(voxelRange, "jet");

  ospCommit(world);

  // create OSPRay renderer
  std::cout << "using renderer: " << argv[1] << std::endl;

  OSPRenderer renderer = ospNewRenderer(argv[1]);

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  VLYVolume vlyVolume = createVolleyVolume();
  ospSetVoidPtr(renderer, "vlyVolume", (void *)vlyVolume);
  ospSetObject(renderer, "transferFunction", tfn);

  ospCommit(renderer);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  box3f bounds(-1.f, 1.f);

  auto glfwOSPRayWindow = std::unique_ptr<GLFWOSPRayWindow>(
      new GLFWOSPRayWindow(vec2i{512, 512}, bounds, world, renderer));

  glfwOSPRayWindow->registerImGuiCallback([=]() {
    static float samplingRate = 1.f;
    if (ImGui::SliderFloat("samplingRate", &samplingRate, 0.01f, 4.f)) {
      std::cerr << "WARNING: samplingRate setting not implemented" << std::endl;
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(world);
  ospRelease(renderer);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
