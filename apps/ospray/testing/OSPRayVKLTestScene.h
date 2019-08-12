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
#include "openvkl_testing.h"
#include "ospray/ospray_testing/ospray_testing.h"

using namespace ospcommon;
using namespace openvkl::testing;

void initializeOSPRay();
void initializeOpenVKL();

OSPVolume convertToOSPVolume(
    std::shared_ptr<TestingStructuredVolume> proceduralVolume,
    OSPTransferFunction transferFunction);

struct OSPRayVKLTestScene
{
  OSPRayVKLTestScene(
      const std::string &rendererType,
      std::shared_ptr<TestingStructuredVolume> proceduralVolume);

  ~OSPRayVKLTestScene();

  void setIsovalues(const std::vector<float> &isovalues);

  OSPWorld getWorld();
  OSPRenderer getRenderer();
  OSPTransferFunction getTransferFunction();

  box3f getBoundingBox();

  std::unique_ptr<OSPRayWindow> getOSPRayWindow(const vec2i &windowSize);

 protected:
  std::shared_ptr<TestingStructuredVolume> proceduralVolume;

  OSPWorld world;
  OSPRenderer renderer;
  OSPTransferFunction transferFunction;
};
