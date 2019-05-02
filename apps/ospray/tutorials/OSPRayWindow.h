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

#include <ospcommon/box.h>
#include <ospcommon/vec.h>
#include <ospray/ospray.h>
#include <functional>
#include "ArcballCamera.h"

class OSPRayWindow
{
 public:
  OSPRayWindow(const ospcommon::vec2i &windowSize,
               const ospcommon::box3f &worldBounds,
               OSPModel model,
               OSPRenderer renderer);

  virtual ~OSPRayWindow() = default;

  OSPModel getModel();
  void setModel(OSPModel newModel);

  void resetAccumulation();

  void registerDisplayCallback(std::function<void(OSPRayWindow *)> callback);

 protected:
  virtual void reshape(const ospcommon::vec2i &newWindowSize);
  virtual void display();

  ospcommon::vec2i windowSize;
  ospcommon::box3f worldBounds;
  OSPModel model       = nullptr;
  OSPRenderer renderer = nullptr;

  std::unique_ptr<ArcballCamera> arcballCamera;

  // OSPRay objects managed by this class
  OSPCamera camera           = nullptr;
  OSPFrameBuffer framebuffer = nullptr;

  // optional registered display callback, called before every display()
  std::function<void(OSPRayWindow *)> displayCallback;
};
