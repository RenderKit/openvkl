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

OSPVolume convertToOSPVolume(
    std::shared_ptr<WaveletProceduralVolume> proceduralVolume,
    OSPTransferFunction transferFunction);

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

    transferFunction =
        ospTestingNewTransferFunction(osp::vec2f{-1.f, 1.f}, "jet");
    ospSetObject(renderer, "transferFunction", transferFunction);
    ospRelease(transferFunction);

    if (rendererType.find("volley") != std::string::npos) {
      ospSetVoidPtr(
          renderer, "vlyVolume", (void *)proceduralVolume->getVLYVolume());
    } else {
      // OSPRay native renderer
      OSPVolume volume = convertToOSPVolume(proceduralVolume, transferFunction);
      ospSetVoidPtr(renderer, "volume", (void *)volume);
    }

    ospCommit(renderer);
  }

  ~OSPRayVolleyTestScene()
  {
    ospRelease(world);
    ospRelease(renderer);
  }

  void setIsovalues(const std::vector<float> &isovalues)
  {
    OSPData isovaluesData =
        ospNewData(isovalues.size(), OSP_FLOAT, isovalues.data());
    ospSetObject(renderer, "isosurfaces", isovaluesData);
    ospRelease(isovaluesData);

    ospCommit(renderer);
  }

  OSPModel getWorld()
  {
    return world;
  }

  OSPRenderer getRenderer()
  {
    return renderer;
  }

  OSPTransferFunction getTransferFunction()
  {
    return transferFunction;
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
  OSPTransferFunction transferFunction;
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

OSPVolume convertToOSPVolume(
    std::shared_ptr<WaveletProceduralVolume> proceduralVolume,
    OSPTransferFunction transferFunction)
{
  OSPVolume volume = ospNewVolume("shared_structured_volume");

  vec3i dimensions  = proceduralVolume->getDimensions();
  vec3f gridOrigin  = proceduralVolume->getGridOrigin();
  vec3f gridSpacing = proceduralVolume->getGridSpacing();

  ospSet3i(volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
  ospSet3f(volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
  ospSet3f(volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

  ospSetString(volume, "voxelType", "float");

  std::vector<float> voxels = proceduralVolume->generateVoxels();

  OSPData voxelData = ospNewData(voxels.size(), OSP_FLOAT, voxels.data());
  ospSetData(volume, "voxelData", voxelData);
  ospRelease(voxelData);

  // required by OSPRay but not used in simple native renderer
  ospSetObject(volume, "transferFunction", transferFunction);

  ospCommit(volume);

  return volume;
}
