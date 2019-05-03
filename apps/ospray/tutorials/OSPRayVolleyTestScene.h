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

#pragma once

#include "OSPRayWindow.h"
#include "ospray/ospray_testing/ospray_testing.h"
#include "volley_testing.h"

using namespace ospcommon;
using namespace volley::testing;

struct OSPRayVolleyTestScene
{
  OSPRayVolleyTestScene(
      const std::string &rendererType,
      std::shared_ptr<WaveletProceduralVolume> proceduralVolume)
      : proceduralVolume(proceduralVolume)
  {
    world = ospNewModel();
    ospCommit(world);

    renderer = ospNewRenderer(rendererType.c_str());

    ospSetVoidPtr(
        renderer, "vlyVolume", (void *)proceduralVolume->getVLYVolume());

    OSPTransferFunction transferFunction =
        ospTestingNewTransferFunction(osp::vec2f{-1.f, 1.f}, "jet");
    ospSetObject(renderer, "transferFunction", transferFunction);
    ospRelease(transferFunction);

    ospCommit(renderer);
  }

  ~OSPRayVolleyTestScene()
  {
    ospRelease(world);
    ospRelease(renderer);
  }

  OSPModel getWorld()
  {
    return world;
  }
  OSPRenderer getRenderer()
  {
    return renderer;
  }
  box3f getBoundingBox()
  {
    vly_box3f boundingBox = vlyGetBoundingBox(proceduralVolume->getVLYVolume());
    return reinterpret_cast<box3f &>(boundingBox);
  }

  std::unique_ptr<OSPRayWindow> getOSPRayWindow(const vec2i &windowSize)
  {
    vly_box3f boundingBox = vlyGetBoundingBox(proceduralVolume->getVLYVolume());

    return std::unique_ptr<OSPRayWindow>(new OSPRayWindow(
        windowSize, reinterpret_cast<box3f &>(boundingBox), world, renderer));
  }

 protected:
  std::shared_ptr<WaveletProceduralVolume> proceduralVolume;

  OSPModel world;
  OSPRenderer renderer;
};

void initializeOSPRay()
{
  static bool initialized = false;

  if (!initialized) {
    OSPError initError = ospInit(nullptr, nullptr);

    if (initError != OSP_NO_ERROR)
      throw std::runtime_error("error initializing OSPRay");

    ospDeviceSetErrorFunc(
        ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
          std::cerr << "OSPRay error: " << errorDetails << std::endl;
          exit(error);
        });

    initialized = true;
  }
}

void initializeVolley()
{
  static bool initialized = false;

  if (!initialized) {
    vlyLoadModule("ispc_driver");

    VLYDriver driver = vlyNewDriver("ispc_driver");
    vlyCommitDriver(driver);
    vlySetCurrentDriver(driver);

    initialized = true;
  }
}
