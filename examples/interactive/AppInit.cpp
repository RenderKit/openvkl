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

#include "AppInit.h"
// openvkl
#include "openvkl/openvkl.h"
// ospcommon
#include "ospcommon/common.h"

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

#if 0
OSPRayVKLTestScene::OSPRayVKLTestScene(
    const std::string &rendererType,
    std::shared_ptr<TestingStructuredVolume> proceduralVolume)
    : proceduralVolume(proceduralVolume)
{
  world = ospNewWorld();
  ospCommit(world);

  renderer = ospNewRenderer(rendererType.c_str());

  transferFunction = ospTestingNewTransferFunction(osp_vec2f{-1.f, 1.f}, "jet");
  ospSetObject(renderer, "transferFunction", transferFunction);
  ospRelease(transferFunction);

  ospSetVoidPtr(
      renderer, "vklVolume", (void *)proceduralVolume->getVKLVolume());

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

std::unique_ptr<OSPRayWindow> OSPRayVKLTestScene::getOSPRayWindow(
    const vec2i &windowSize)
{
  vkl_box3f boundingBox = vklGetBoundingBox(proceduralVolume->getVKLVolume());

  return std::unique_ptr<OSPRayWindow>(new OSPRayWindow(
      windowSize, reinterpret_cast<box3f &>(boundingBox), world, renderer));
}
#endif
