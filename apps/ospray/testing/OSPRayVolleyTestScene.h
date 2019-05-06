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

void initializeOSPRay();
void initializeVolley();

OSPVolume convertToOSPVolume(
    std::shared_ptr<WaveletProceduralVolume> proceduralVolume,
    OSPTransferFunction transferFunction);

struct OSPRayVolleyTestScene
{
  OSPRayVolleyTestScene(
      const std::string &rendererType,
      std::shared_ptr<WaveletProceduralVolume> proceduralVolume);

  ~OSPRayVolleyTestScene();

  void setIsovalues(const std::vector<float> &isovalues);

  OSPModel getWorld();
  OSPRenderer getRenderer();
  OSPTransferFunction getTransferFunction();

  box3f getBoundingBox();

  std::unique_ptr<OSPRayWindow> getOSPRayWindow(const vec2i &windowSize);

 protected:
  std::shared_ptr<WaveletProceduralVolume> proceduralVolume;

  OSPModel world;
  OSPRenderer renderer;
  OSPTransferFunction transferFunction;
};
