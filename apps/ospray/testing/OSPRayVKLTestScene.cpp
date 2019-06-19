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

#include "OSPRayVKLTestScene.h"

//////////////////////////////////////////////////////////////////////////////
// Helper functions //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void initializeOSPRay()
{
  static bool initialized = false;

  if (!initialized) {
    // NOTE(jda) - Workaround issue with not exposing 'anchor' feature through
    //             ospLoadModule().
    ospcommon::loadLibrary("ospray_module_ispc_openvkl", false);
    ospLoadModule("ispc_openvkl");

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

void initializeOpenVKL()
{
  static bool initialized = false;

  if (!initialized) {
    // NOTE(jda) - Workaround issue with not exposing 'anchor' feature through
    //             vklLoadModule().
    ospcommon::loadLibrary("openvkl_module_ispc_driver", false);
    vklLoadModule("ispc_driver");

    VKLDriver driver = vklNewDriver("ispc_driver");
    vklCommitDriver(driver);
    vklSetCurrentDriver(driver);

    initialized = true;
  }
}

OSPVolume convertToOSPVolume(
    std::shared_ptr<TestingStructuredVolume> proceduralVolume,
    OSPTransferFunction transferFunction)
{
  OSPVolume volume = ospNewVolume("shared_structured_volume");

  vec3i dimensions  = proceduralVolume->getDimensions();
  vec3f gridOrigin  = proceduralVolume->getGridOrigin();
  vec3f gridSpacing = proceduralVolume->getGridSpacing();

  ospSetVec3i(volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
  ospSetVec3f(volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
  ospSetVec3f(
      volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

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

///////////////////////////////////////////////////////////////////////////////
// OSPRayVKLTestScene /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

OSPRayVKLTestScene::OSPRayVKLTestScene(
    const std::string &rendererType,
    std::shared_ptr<TestingStructuredVolume> proceduralVolume)
    : proceduralVolume(proceduralVolume)
{
  world = ospNewWorld();
  ospCommit(world);

  renderer = ospNewRenderer(rendererType.c_str());

  transferFunction =
      ospTestingNewTransferFunction(osp_vec2f{-1.f, 1.f}, "jet");
  ospSetObject(renderer, "transferFunction", transferFunction);
  ospRelease(transferFunction);

  if (rendererType.find("vkl") != std::string::npos) {
    ospSetVoidPtr(
        renderer, "vklVolume", (void *)proceduralVolume->getVKLVolume());
  } else {
    // OSPRay native renderer
    OSPVolume volume = convertToOSPVolume(proceduralVolume, transferFunction);
    ospSetVoidPtr(renderer, "volume", (void *)volume);
  }

  ospCommit(renderer);
}

OSPRayVKLTestScene::~OSPRayVKLTestScene()
{
  ospRelease(world);
  ospRelease(renderer);
}

void OSPRayVKLTestScene::setIsovalues(const std::vector<float> &isovalues)
{
  OSPData isovaluesData =
      ospNewData(isovalues.size(), OSP_FLOAT, isovalues.data());
  ospSetObject(renderer, "isosurfaces", isovaluesData);
  ospRelease(isovaluesData);

  ospCommit(renderer);
}

OSPWorld OSPRayVKLTestScene::getWorld()
{
  return world;
}

OSPRenderer OSPRayVKLTestScene::getRenderer()
{
  return renderer;
}

OSPTransferFunction OSPRayVKLTestScene::getTransferFunction()
{
  return transferFunction;
}

box3f OSPRayVKLTestScene::getBoundingBox()
{
  vkl_box3f boundingBox = vklGetBoundingBox(proceduralVolume->getVKLVolume());
  return reinterpret_cast<box3f &>(boundingBox);
}

std::unique_ptr<OSPRayWindow> OSPRayVKLTestScene::getOSPRayWindow(
    const vec2i &windowSize)
{
  vkl_box3f boundingBox = vklGetBoundingBox(proceduralVolume->getVKLVolume());

  return std::unique_ptr<OSPRayWindow>(new OSPRayWindow(
      windowSize, reinterpret_cast<box3f &>(boundingBox), world, renderer));
}
