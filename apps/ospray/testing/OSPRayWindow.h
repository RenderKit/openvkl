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

#include <unordered_map>
#include "ArcballCamera.h"
#include "ospcommon/math/box.h"
#include "ospcommon/math/vec.h"
#include "ospray/ospray.h"

using namespace ospcommon::math;

class OSPRayWindow
{
 public:
  OSPRayWindow(const vec2i &windowSize,
               const box3f &worldBounds,
               OSPWorld world,
               OSPRenderer renderer);

  virtual ~OSPRayWindow() = default;

  OSPWorld getWorld();
  void setWorld(OSPWorld newWorld);

  void setTimestep(int timestep);

  void setPauseRendering(bool set);

  void render();

  void resetAccumulation();

  void savePPM(const std::string &filename);

 protected:
  virtual void reshape(const vec2i &newWindowSize);

  vec2i windowSize;
  box3f worldBounds;
  OSPWorld world       = nullptr;
  OSPRenderer renderer = nullptr;

  std::unique_ptr<ArcballCamera> arcballCamera;

  // OSPRay objects managed by this class
  OSPCamera camera           = nullptr;
  OSPFrameBuffer framebuffer = nullptr;

  // frame buffers can be tracked per time step. accumulation will not be reset
  // when changing time steps, only when resetAccumulation() is called
  // explicitly. this allows time series renderings to continually refine over
  // multiple playback loops, for example.
  int currentTimestep = 0;
  std::unordered_map<int, OSPFrameBuffer> framebuffersPerTimestep;

  // allow window to redraw but do not render new frames (display current
  // framebuffer only)
  bool pauseRendering = false;
};
