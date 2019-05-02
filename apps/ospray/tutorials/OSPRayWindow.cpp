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

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"

OSPRayWindow::OSPRayWindow(const ospcommon::vec2i &windowSize,
                           const ospcommon::box3f &worldBounds,
                           OSPModel model,
                           OSPRenderer renderer)
    : windowSize(windowSize),
      worldBounds(worldBounds),
      model(model),
      renderer(renderer)
{
  ospSetObject(renderer, "model", model);

  arcballCamera = std::unique_ptr<ArcballCamera>(
      new ArcballCamera(worldBounds, windowSize));

  camera = ospNewCamera("perspective");
  ospSetf(camera, "aspect", windowSize.x / float(windowSize.y));

  ospSetVec3f(camera,
              "pos",
              osp::vec3f{arcballCamera->eyePos().x,
                         arcballCamera->eyePos().y,
                         arcballCamera->eyePos().z});
  ospSetVec3f(camera,
              "dir",
              osp::vec3f{arcballCamera->lookDir().x,
                         arcballCamera->lookDir().y,
                         arcballCamera->lookDir().z});
  ospSetVec3f(camera,
              "up",
              osp::vec3f{arcballCamera->upDir().x,
                         arcballCamera->upDir().y,
                         arcballCamera->upDir().z});

  ospCommit(camera);

  ospSetObject(renderer, "camera", camera);

  ospCommit(renderer);

  reshape(this->windowSize);
}

OSPModel OSPRayWindow::getModel()
{
  return model;
}

void OSPRayWindow::setModel(OSPModel newModel)
{
  model = newModel;

  ospSetObject(renderer, "model", model);
  ospCommit(renderer);

  resetAccumulation();
}

void OSPRayWindow::resetAccumulation()
{
  ospFrameBufferClear(framebuffer, OSP_FB_COLOR | OSP_FB_ACCUM);
}

void OSPRayWindow::registerDisplayCallback(
    std::function<void(OSPRayWindow *)> callback)
{
  displayCallback = callback;
}

void OSPRayWindow::reshape(const ospcommon::vec2i &newWindowSize)
{
  windowSize = newWindowSize;

  if (framebuffer)
    ospRelease(framebuffer);

  framebuffer = ospNewFrameBuffer(*reinterpret_cast<osp::vec2i *>(&windowSize),
                                  OSP_FB_SRGBA,
                                  OSP_FB_COLOR | OSP_FB_ACCUM);

  // update camera
  arcballCamera->updateWindowSize(windowSize);

  ospSetf(camera, "aspect", windowSize.x / float(windowSize.y));
  ospCommit(camera);
}

void OSPRayWindow::display()
{
  ospRenderFrame(framebuffer, renderer, OSP_FB_COLOR | OSP_FB_ACCUM);
}
