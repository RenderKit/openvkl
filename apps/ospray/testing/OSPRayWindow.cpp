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

#include "OSPRayWindow.h"
#include <iostream>
#include <stdexcept>
#include "ospcommon/utility/SaveImage.h"

OSPRayWindow::OSPRayWindow(const vec2i &windowSize,
                           const box3f &worldBounds,
                           OSPWorld world,
                           OSPRenderer renderer)
    : windowSize(windowSize),
      worldBounds(worldBounds),
      world(world),
      renderer(renderer)
{
  ospSetObject(renderer, "world", world);

  arcballCamera = std::unique_ptr<ArcballCamera>(
      new ArcballCamera(worldBounds, windowSize));

  camera = ospNewCamera("perspective");
  ospSetFloat(camera, "aspect", windowSize.x / float(windowSize.y));

  ospSetVec3f(camera,
              "position",
              arcballCamera->eyePos().x,
              arcballCamera->eyePos().y,
              arcballCamera->eyePos().z);
  ospSetVec3f(camera,
              "direction",
              arcballCamera->lookDir().x,
              arcballCamera->lookDir().y,
              arcballCamera->lookDir().z);
  ospSetVec3f(camera,
              "up",
              arcballCamera->upDir().x,
              arcballCamera->upDir().y,
              arcballCamera->upDir().z);

  ospCommit(camera);

  ospSetObject(renderer, "camera", camera);

  ospCommit(renderer);

  reshape(this->windowSize);
}

OSPWorld OSPRayWindow::getWorld()
{
  return world;
}

void OSPRayWindow::setWorld(OSPWorld newWorld)
{
  world = newWorld;
  resetAccumulation();
}

void OSPRayWindow::setTimestep(int timestep)
{
  if (currentTimestep == timestep) {
    return;
  }

  currentTimestep = timestep;

  if (framebuffersPerTimestep.count(currentTimestep) == 0) {
    framebuffersPerTimestep[currentTimestep] = ospNewFrameBuffer(
        windowSize.x, windowSize.y, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
  }

  framebuffer = framebuffersPerTimestep[currentTimestep];
}

void OSPRayWindow::setPauseRendering(bool set)
{
  pauseRendering = set;
}

void OSPRayWindow::render()
{
  if (!pauseRendering) {
    ospRenderFrame(framebuffer, renderer, camera, world);
  }
}

void OSPRayWindow::resetAccumulation()
{
  for (auto &kv : framebuffersPerTimestep) {
    ospResetAccumulation(kv.second);
  }
}

void OSPRayWindow::resetCamera()
{
  arcballCamera->resetCamera(worldBounds);
  updateCamera();
}

void OSPRayWindow::updateCamera()
{
  resetAccumulation();

  ospSetFloat(camera, "aspect", windowSize.x / float(windowSize.y));
  ospSetVec3f(camera,
              "pos",
              arcballCamera->eyePos().x,
              arcballCamera->eyePos().y,
              arcballCamera->eyePos().z);
  ospSetVec3f(camera,
              "dir",
              arcballCamera->lookDir().x,
              arcballCamera->lookDir().y,
              arcballCamera->lookDir().z);
  ospSetVec3f(camera,
              "up",
              arcballCamera->upDir().x,
              arcballCamera->upDir().y,
              arcballCamera->upDir().z);

  ospCommit(camera);
}

void OSPRayWindow::savePPM(const std::string &filename)
{
  uint32_t *fb = (uint32_t *)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
  ospcommon::utility::writePPM(filename, windowSize.x, windowSize.y, fb);
  ospUnmapFrameBuffer(fb, framebuffer);
}

void OSPRayWindow::reshape(const vec2i &newWindowSize)
{
  windowSize = newWindowSize;

  for (auto &kv : framebuffersPerTimestep) {
    ospRelease(kv.second);
  }

  framebuffersPerTimestep.clear();

  framebuffersPerTimestep[currentTimestep] = ospNewFrameBuffer(
      windowSize.x, windowSize.y, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);

  framebuffer = framebuffersPerTimestep[currentTimestep];

  // update camera
  arcballCamera->updateWindowSize(windowSize);

  ospSetFloat(camera, "aspect", windowSize.x / float(windowSize.y));
  ospCommit(camera);
}
