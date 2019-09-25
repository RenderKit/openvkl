// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#include "ospcommon/math/AffineSpace.h"

using namespace ospcommon::math;

class ArcballCamera
{
 public:
  ArcballCamera(const box3f &worldBounds, const vec2i &windowSize);

  // All mouse positions passed should be in [-1, 1] normalized screen coords
  void rotate(const vec2f &from, const vec2f &to);
  void zoom(float amount);
  void pan(const vec2f &delta);

  vec3f eyePos() const;
  vec3f center() const;
  vec3f lookDir() const;
  vec3f upDir() const;

  void resetCamera(const box3f &worldBounds);

  void updateWindowSize(const vec2i &windowSize);

 protected:
  void updateCamera();

  // Project the point in [-1, 1] screen space onto the arcball sphere
  quaternionf screenToArcball(const vec2f &p);

  float zoomSpeed;
  vec2f invWindowSize;
  AffineSpace3f centerTranslation, translation, invCamera;
  quaternionf rotation;
};
